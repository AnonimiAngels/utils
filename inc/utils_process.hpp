#pragma once
#ifndef PROCESS_HPP
	#define PROCESS_HPP

	#include <algorithm>
	#include <array>
	#include <atomic>
	#include <chrono>
	#include <condition_variable>
	#include <csignal>
	#include <cstdint>
	#include <functional>
	#include <future>
	#include <map>
	#include <memory>
	#include <mutex>
	#include <string>
	#include <thread>
	#include <utility>
	#include <vector>

	#include <fcntl.h>
	#include <sys/select.h>
	#include <sys/wait.h>
	#include <unistd.h>

// =============================================================================
// CONSTANTS AND CONFIGURATION
// =============================================================================
namespace utils
{
	namespace process_consts
	{
		static const std::int32_t k_exec_error_code	   = 127;
		static const std::size_t k_buffer_size		   = 8192;	  // Increased for better performance
		static const std::size_t k_pipe_count		   = 2;
		static const mode_t k_file_permissions		   = 0644;
		static const auto k_select_timeout_us		   = std::chrono::microseconds(5000);	  // Reduced for responsiveness
		static const auto k_sleep_interval_ms		   = std::chrono::milliseconds(50);		  // Reduced overhead
		static const auto k_main_loop_sleep_ms		   = std::chrono::milliseconds(5);		  // More responsive
		static const auto k_pid_wait_timeout_ms		   = std::chrono::milliseconds(10000);	  // Increased timeout
		static const auto k_termination_timeout_ms	   = std::chrono::milliseconds(5000);	  // Increased timeout
		static const auto k_graceful_term_timeout_ms   = std::chrono::milliseconds(2000);	  // Increased timeout
		static const auto k_default_process_timeout_ms = std::chrono::milliseconds(0);
		static const auto k_terminate_retry_sleep_ms   = std::chrono::milliseconds(10);
		static const auto k_future_wait_timeout_ms	   = std::chrono::milliseconds(2000);	 // Increased timeout
		static const auto k_termination_check_ms	   = std::chrono::milliseconds(10);		 // More responsive
		static const auto k_stdin_write_retry_ms	   = std::chrono::milliseconds(5);
		static const std::int32_t k_max_idle_cycles	   = 50;	// Reduced for responsiveness
		static const std::int32_t k_max_drain_attempts = 20;	// Increased thoroughness
	}	 // namespace process_consts

	// =============================================================================
	// FORWARD DECLARATIONS AND TYPE ALIASES
	// =============================================================================

	class process;

	namespace process_detail
	{
		using pipe_t	= std::array<std::int32_t, process_consts::k_pipe_count>;
		using data_t	= std::array<char, process_consts::k_buffer_size>;
		using env_map_t = std::map<std::string, std::string>;

		// =============================================================================
		// RAII PIPE WRAPPER
		// =============================================================================

		class pipe_wrapper
		{
		private:
			pipe_t m_pipe;
			bool m_read_open{};
			bool m_write_open{};

		public:
			pipe_wrapper() : m_pipe{{-1, -1}}
			{
				if (pipe(m_pipe.data()) == 0)
				{
					m_read_open	 = true;
					m_write_open = true;
				}
			}

			~pipe_wrapper()
			{
				close_read();
				close_write();
			}

			pipe_wrapper(const pipe_wrapper&)					 = delete;
			auto operator=(const pipe_wrapper&) -> pipe_wrapper& = delete;

			pipe_wrapper(pipe_wrapper&& p_other) noexcept
				: m_pipe(p_other.m_pipe), m_read_open(p_other.m_read_open), m_write_open(p_other.m_write_open)
			{
				p_other.m_pipe[0]	 = -1;
				p_other.m_pipe[1]	 = -1;
				p_other.m_read_open	 = false;
				p_other.m_write_open = false;
			}

			auto operator=(pipe_wrapper&& p_other) noexcept -> pipe_wrapper&
			{
				if (this != &p_other)
				{
					close_read();
					close_write();
					m_pipe		 = p_other.m_pipe;
					m_read_open	 = p_other.m_read_open;
					m_write_open = p_other.m_write_open;

					p_other.m_pipe[0]	 = -1;
					p_other.m_pipe[1]	 = -1;
					p_other.m_read_open	 = false;
					p_other.m_write_open = false;
				}
				return *this;
			}

			auto is_valid() const -> bool { return m_pipe[0] != -1 && m_pipe[1] != -1; }

			auto read_fd() const -> std::int32_t { return m_pipe[0]; }

			auto write_fd() const -> std::int32_t { return m_pipe[1]; }

			auto close_read() -> void
			{
				if (m_read_open && m_pipe[0] != -1)
				{
					close(m_pipe[0]);
					m_read_open = false;
				}
			}

			auto close_write() -> void
			{
				if (m_write_open && m_pipe[1] != -1)
				{
					close(m_pipe[1]);
					m_write_open = false;
				}
			}

			auto get_pipe() const -> const pipe_t& { return m_pipe; }
		};

		// =============================================================================
		// BUFFER POOL FOR MEMORY EFFICIENCY
		// =============================================================================

		class buffer_pool
		{
		private:
			std::vector<std::unique_ptr<data_t>> m_buffers;
			std::mutex m_mutex;
			static const std::size_t k_max_pool_size = 32;

		public:
			auto acquire() -> std::unique_ptr<data_t>
			{
				const std::lock_guard<std::mutex> lock(m_mutex);
				if (!m_buffers.empty())
				{
					std::unique_ptr<data_t> buffer = std::move(m_buffers.back());
					m_buffers.pop_back();
					return buffer;
				}
				return std::unique_ptr<data_t>(new data_t());
			}

			auto release(std::unique_ptr<data_t> p_buffer) -> void
			{
				if (!p_buffer)
				{
					return;
				}

				const std::lock_guard<std::mutex> lock(m_mutex);
				if (m_buffers.size() < k_max_pool_size)
				{
					m_buffers.push_back(std::move(p_buffer));
				}
			}

			static auto instance() -> buffer_pool&
			{
				static buffer_pool pool;
				return pool;
			}
		};

		// =============================================================================
		// PROCESS UTILITIES
		// =============================================================================

		class process_utilities
		{
		public:
			static auto check_process_running(const std::string& p_process_name) -> bool
			{
				const std::string command = "pgrep -x \"" + p_process_name + "\"";
				FILE* pipe				  = popen(command.c_str(), "r");
				if (pipe == nullptr)
				{
					return false;
				}

				constexpr std::size_t k_max_output_size = 128;
				std::array<char, k_max_output_size> buffer{};
				const bool is_running = (fgets(buffer.data(), static_cast<std::int32_t>(buffer.size()), pipe) != nullptr);
				pclose(pipe);
				return is_running;
			}

			static auto set_fd_nonblocking(std::int32_t p_file_descriptor) -> bool
			{
				const std::int32_t flags = fcntl(p_file_descriptor, F_GETFL, 0);
				if (flags == -1)
				{
					return false;
				}
				return fcntl(p_file_descriptor, F_SETFL, flags | O_NONBLOCK) != -1;
			}

			static auto read_file_content(const std::string& p_file_path, std::string& p_content) -> bool
			{
				const std::int32_t file_descriptor = open(p_file_path.c_str(), O_RDONLY);
				if (file_descriptor == -1)
				{
					return false;
				}

				constexpr std::size_t k_chunk_size = 4096;
				std::array<char, k_chunk_size> buffer{};
				ssize_t bytes_read = 0;

				while ((bytes_read = read(file_descriptor, buffer.data(), buffer.size())) > 0)
				{
					p_content.append(buffer.data(), static_cast<std::size_t>(bytes_read));
				}

				close(file_descriptor);
				return bytes_read != -1;
			}
		};

		template <typename T> class span
		{
		public:
			using element_type	  = T;
			using value_type	  = typename std::remove_cv<T>::type;
			using size_type		  = std::size_t;
			using difference_type = std::ptrdiff_t;
			using pointer		  = T*;
			using const_pointer	  = const T*;
			using reference		  = T&;
			using const_reference = const T&;
			using iterator		  = T*;
			using const_iterator  = const T*;

		private:
			pointer m_data;
			size_type m_size;

		public:
			span() : m_data(nullptr), m_size(0) {}

			span(pointer p_data, size_type p_size) : m_data(p_data), m_size(p_size) {}

			span(pointer p_data, difference_type p_offset, size_type p_size) : m_data(std::next(p_data, p_offset)), m_size(p_size) {}

			span(const span& p_other) : m_data(p_other.m_data), m_size(p_other.m_size) {}

			span(span&& p_other) noexcept : m_data(p_other.m_data), m_size(p_other.m_size)
			{
				p_other.m_data = nullptr;
				p_other.m_size = 0;
			}

			auto operator=(const span& p_other) -> span&
			{
				if (this != &p_other)
				{
					m_data = p_other.m_data;
					m_size = p_other.m_size;
				}
				return *this;
			}

			auto operator=(span&& p_other) noexcept -> span&
			{
				if (this != &p_other)
				{
					m_data		   = p_other.m_data;
					m_size		   = p_other.m_size;
					p_other.m_data = nullptr;
					p_other.m_size = 0;
				}
				return *this;
			}

			~span() = default;

			auto begin() const -> iterator { return m_data; }

			auto end() const -> iterator { return m_data + m_size; }

			auto cbegin() const -> const_iterator { return m_data; }

			auto cend() const -> const_iterator { return m_data + m_size; }

			auto operator[](size_type p_index) const -> reference { return m_data[p_index]; }

			auto at(size_type p_index) const -> reference { return m_data[p_index]; }

			auto front() const -> reference { return m_data[0]; }

			auto back() const -> reference { return m_data[m_size - 1]; }

			auto data() const -> pointer { return m_data; }

			auto size() const -> size_type { return m_size; }

			auto size_bytes() const -> size_type { return m_size * sizeof(element_type); }

			auto empty() const -> bool { return m_size == 0; }

			auto subspan(size_type p_offset, size_type p_count = static_cast<size_type>(-1)) const -> span
			{
				size_type actual_count = (p_count == static_cast<size_type>(-1)) ? (m_size - p_offset) : p_count;
				return span(m_data + p_offset, actual_count);
			}

			auto first(size_type p_count) const -> span { return subspan(0, p_count); }

			auto last(size_type p_count) const -> span { return subspan(m_size - p_count, p_count); }
		};

		template <typename T> auto make_span(T* p_data, std::size_t p_size) -> span<T>
		{
			return span<T>(p_data, p_size);
		}
	}	 // namespace process_detail

	// =============================================================================
	// MAIN PROCESS CLASS
	// =============================================================================

	class process
	{
	public:
		// =============================================================================
		// PUBLIC ENUMS AND TYPES
		// =============================================================================

		enum class status_t : std::uint8_t
		{
			not_started,
			running,
			finished,
			terminated,
			timeout,
			error
		};

		enum class error_code_t : std::uint8_t
		{
			none,
			fork_failed,
			pipe_creation_failed,
			exec_failed,
			working_dir_failed,
			env_setup_failed,
			stdin_write_failed,
			termination_failed,
			timeout_exceeded,
			invalid_state,
			system_error,
			insufficient_privileges,
			resource_limit_exceeded
		};

		enum class redirect_mode_t : std::uint8_t
		{
			none,
			memory,
			file,
			both
		};

		struct termination_request_t
		{
			std::atomic<bool> requested{false};
			std::atomic<bool> force{false};
			std::atomic<bool> completed{false};
			std::chrono::steady_clock::time_point request_time;
		};

		using output_callback_t = std::function<auto(const std::string&)->void>;

	private:
		// =============================================================================
		// PRIVATE MEMBER VARIABLES
		// =============================================================================

		std::unique_ptr<std::mutex> m_output_mutex;
		std::string m_stdout_buffer;
		std::string m_stderr_buffer;
		std::string m_stdin_buffer;
		std::string m_working_directory;
		std::string m_command;
		process_detail::env_map_t m_environment;
		std::int32_t m_return_code = -1;
		std::atomic<pid_t> m_pid;
		std::atomic<status_t> m_status;
		std::atomic<error_code_t> m_last_error;
		std::future<void> m_future;
		std::chrono::milliseconds m_timeout;
		std::chrono::milliseconds m_graceful_termination_timeout;
		output_callback_t m_stdout_callback;
		output_callback_t m_stderr_callback;
		bool m_capture_output = true;

		std::string m_stdin_file_path;
		std::string m_stdout_file_path;
		std::string m_stderr_file_path;
		redirect_mode_t m_stdout_redirect_mode = redirect_mode_t::memory;
		redirect_mode_t m_stderr_redirect_mode = redirect_mode_t::memory;
		bool m_stdin_from_file				   = false;

		std::unique_ptr<std::mutex> m_pid_mutex;
		std::unique_ptr<std::condition_variable> m_pid_condition;
		std::atomic<bool> m_pid_ready;

		termination_request_t m_termination_request;

		// =============================================================================
		// FUTURE WAITER HELPER CLASS
		// =============================================================================

		class future_waiter
		{
		private:
			std::future<void> m_future;

		public:
			~future_waiter() = default;

			explicit future_waiter(std::future<void>&& p_future) : m_future(std::move(p_future)) {}

			future_waiter(const future_waiter&)					   = delete;
			auto operator=(const future_waiter&) -> future_waiter& = delete;

			future_waiter(future_waiter&& p_other) noexcept : m_future(std::move(p_other.m_future)) {}

			auto operator=(future_waiter&& p_other) noexcept -> future_waiter&
			{
				if (this != &p_other)
				{
					m_future = std::move(p_other.m_future);
				}
				return *this;
			}

			auto operator()() -> void
			{
				if (m_future.valid())
				{
					m_future.wait();
				}
			}
		};

	public:
		// =============================================================================
		// CONSTRUCTORS AND DESTRUCTOR
		// =============================================================================

		process()
			: m_output_mutex(new std::mutex()),
			  m_pid(-1),
			  m_status(status_t::not_started),
			  m_last_error(error_code_t::none),
			  m_timeout(process_consts::k_default_process_timeout_ms),
			  m_graceful_termination_timeout(process_consts::k_graceful_term_timeout_ms),
			  m_pid_mutex(new std::mutex()),
			  m_pid_condition(new std::condition_variable()),
			  m_pid_ready(false)
		{
		}

		explicit process(const std::string& p_command)
			: m_output_mutex(new std::mutex()),
			  m_pid(-1),
			  m_status(status_t::not_started),
			  m_last_error(error_code_t::none),
			  m_timeout(process_consts::k_default_process_timeout_ms),
			  m_graceful_termination_timeout(process_consts::k_graceful_term_timeout_ms),
			  m_pid_mutex(new std::mutex()),
			  m_pid_condition(new std::condition_variable()),
			  m_pid_ready(false)
		{
			execute(p_command);
		}

		process(const std::string& p_command, std::string p_working_dir)
			: m_output_mutex(new std::mutex()),
			  m_working_directory(std::move(p_working_dir)),
			  m_pid(-1),
			  m_status(status_t::not_started),
			  m_last_error(error_code_t::none),
			  m_timeout(process_consts::k_default_process_timeout_ms),
			  m_graceful_termination_timeout(process_consts::k_graceful_term_timeout_ms),
			  m_pid_mutex(new std::mutex()),
			  m_pid_condition(new std::condition_variable()),
			  m_pid_ready(false)
		{
			execute(p_command);
		}

		~process()
		{
			terminate_sync(true);
			wait_with_timeout();
		}

		// Copy operations (deleted)
		process(const process& p_other)					   = delete;
		auto operator=(const process& p_other) -> process& = delete;

		// Move operations
		process(process&& p_other) noexcept
			: m_output_mutex(std::move(p_other.m_output_mutex)),
			  m_stdout_buffer(std::move(p_other.m_stdout_buffer)),
			  m_stderr_buffer(std::move(p_other.m_stderr_buffer)),
			  m_stdin_buffer(std::move(p_other.m_stdin_buffer)),
			  m_working_directory(std::move(p_other.m_working_directory)),
			  m_command(std::move(p_other.m_command)),
			  m_environment(std::move(p_other.m_environment)),
			  m_return_code(p_other.m_return_code),
			  m_pid(p_other.m_pid.load()),
			  m_status(p_other.m_status.load()),
			  m_last_error(p_other.m_last_error.load()),
			  m_future(std::move(p_other.m_future)),
			  m_timeout(p_other.m_timeout),
			  m_graceful_termination_timeout(p_other.m_graceful_termination_timeout),
			  m_stdout_callback(std::move(p_other.m_stdout_callback)),
			  m_stderr_callback(std::move(p_other.m_stderr_callback)),
			  m_capture_output(p_other.m_capture_output),
			  m_stdin_file_path(std::move(p_other.m_stdin_file_path)),
			  m_stdout_file_path(std::move(p_other.m_stdout_file_path)),
			  m_stderr_file_path(std::move(p_other.m_stderr_file_path)),
			  m_stdout_redirect_mode(p_other.m_stdout_redirect_mode),
			  m_stderr_redirect_mode(p_other.m_stderr_redirect_mode),
			  m_stdin_from_file(p_other.m_stdin_from_file),
			  m_pid_mutex(std::move(p_other.m_pid_mutex)),
			  m_pid_condition(std::move(p_other.m_pid_condition)),
			  m_pid_ready(p_other.m_pid_ready.load())
		{
			// Manually move atomic members of termination_request
			m_termination_request.requested.store(p_other.m_termination_request.requested.load());
			m_termination_request.force.store(p_other.m_termination_request.force.load());
			m_termination_request.completed.store(p_other.m_termination_request.completed.load());
			m_termination_request.request_time = p_other.m_termination_request.request_time;

			// Reset moved-from object
			p_other.m_return_code = -1;
			p_other.m_pid.store(-1);
			p_other.m_status.store(status_t::not_started);
			p_other.m_last_error.store(error_code_t::none);
			p_other.m_timeout					   = process_consts::k_default_process_timeout_ms;
			p_other.m_graceful_termination_timeout = process_consts::k_graceful_term_timeout_ms;
			p_other.m_capture_output			   = true;
			p_other.m_stdout_redirect_mode		   = redirect_mode_t::memory;
			p_other.m_stderr_redirect_mode		   = redirect_mode_t::memory;
			p_other.m_stdin_from_file			   = false;
			p_other.m_pid_ready.store(false);
			p_other.m_termination_request.requested.store(false);
			p_other.m_termination_request.force.store(false);
			p_other.m_termination_request.completed.store(false);
		}

		auto operator=(process&& p_other) noexcept -> process&
		{
			if (this != &p_other)
			{
				// Cleanup current object
				terminate_sync(true);
				wait_with_timeout();

				// Move data
				m_output_mutex		= std::move(p_other.m_output_mutex);
				m_stdout_buffer		= std::move(p_other.m_stdout_buffer);
				m_stderr_buffer		= std::move(p_other.m_stderr_buffer);
				m_stdin_buffer		= std::move(p_other.m_stdin_buffer);
				m_working_directory = std::move(p_other.m_working_directory);
				m_command			= std::move(p_other.m_command);
				m_environment		= std::move(p_other.m_environment);
				m_return_code		= p_other.m_return_code;
				m_pid.store(p_other.m_pid.load());
				m_status.store(p_other.m_status.load());
				m_last_error.store(p_other.m_last_error.load());
				m_future					   = std::move(p_other.m_future);
				m_timeout					   = p_other.m_timeout;
				m_graceful_termination_timeout = p_other.m_graceful_termination_timeout;
				m_stdout_callback			   = std::move(p_other.m_stdout_callback);
				m_stderr_callback			   = std::move(p_other.m_stderr_callback);
				m_capture_output			   = p_other.m_capture_output;
				m_stdin_file_path			   = std::move(p_other.m_stdin_file_path);
				m_stdout_file_path			   = std::move(p_other.m_stdout_file_path);
				m_stderr_file_path			   = std::move(p_other.m_stderr_file_path);
				m_stdout_redirect_mode		   = p_other.m_stdout_redirect_mode;
				m_stderr_redirect_mode		   = p_other.m_stderr_redirect_mode;
				m_stdin_from_file			   = p_other.m_stdin_from_file;
				m_pid_mutex					   = std::move(p_other.m_pid_mutex);
				m_pid_condition				   = std::move(p_other.m_pid_condition);
				m_pid_ready.store(p_other.m_pid_ready.load());

				// Manually move atomic members of termination_request
				m_termination_request.requested.store(p_other.m_termination_request.requested.load());
				m_termination_request.force.store(p_other.m_termination_request.force.load());
				m_termination_request.completed.store(p_other.m_termination_request.completed.load());
				m_termination_request.request_time = p_other.m_termination_request.request_time;

				// Reset moved-from object
				p_other.m_return_code = -1;
				p_other.m_pid.store(-1);
				p_other.m_status.store(status_t::not_started);
				p_other.m_last_error.store(error_code_t::none);
				p_other.m_timeout					   = process_consts::k_default_process_timeout_ms;
				p_other.m_graceful_termination_timeout = process_consts::k_graceful_term_timeout_ms;
				p_other.m_capture_output			   = true;
				p_other.m_stdout_redirect_mode		   = redirect_mode_t::memory;
				p_other.m_stderr_redirect_mode		   = redirect_mode_t::memory;
				p_other.m_stdin_from_file			   = false;
				p_other.m_pid_ready.store(false);
				p_other.m_termination_request.requested.store(false);
				p_other.m_termination_request.force.store(false);
				p_other.m_termination_request.completed.store(false);
			}
			return *this;
		}

		// =============================================================================
		// PUBLIC GETTERS
		// =============================================================================

		auto get_stdout() const -> std::string
		{
			const std::lock_guard<std::mutex> lock(*m_output_mutex);
			return m_stdout_buffer;
		}

		auto get_stderr() const -> std::string
		{
			const std::lock_guard<std::mutex> lock(*m_output_mutex);
			return m_stderr_buffer;
		}

		auto get_command() const -> std::string
		{
			const std::lock_guard<std::mutex> lock(*m_output_mutex);
			return m_command;
		}

		auto get_return_code() const -> std::int32_t { return m_return_code; }

		auto get_pid() const -> pid_t { return m_pid.load(); }

		auto get_status() const -> status_t { return m_status.load(); }

		auto get_last_error() const -> error_code_t { return m_last_error.load(); }

		auto get_error_message() const -> std::string { return error_code_to_string(m_last_error.load()); }

		auto get_working_directory() const -> const std::string& { return m_working_directory; }

		auto get_timeout() const -> std::chrono::milliseconds { return m_timeout; }

		// =============================================================================
		// PUBLIC SETTERS
		// =============================================================================

		auto set_stdin(std::string p_input) -> void
		{
			m_stdin_buffer	  = std::move(p_input);
			m_stdin_from_file = false;
		}

		auto set_stdin_file(const std::string& p_file_path) -> void
		{
			m_stdin_file_path = p_file_path;
			m_stdin_from_file = true;
			m_stdin_buffer.clear();
		}

		auto set_stdout_file(const std::string& p_file_path, redirect_mode_t p_mode = redirect_mode_t::file) -> void
		{
			m_stdout_file_path	   = p_file_path;
			m_stdout_redirect_mode = p_mode;
		}

		auto set_stderr_file(const std::string& p_file_path, redirect_mode_t p_mode = redirect_mode_t::file) -> void
		{
			m_stderr_file_path	   = p_file_path;
			m_stderr_redirect_mode = p_mode;
		}

		auto set_working_directory(const std::string& p_directory) -> void { m_working_directory = p_directory; }

		auto set_timeout(std::chrono::milliseconds p_timeout) -> void { m_timeout = p_timeout; }

		auto set_graceful_termination_timeout(std::chrono::milliseconds p_timeout) -> void { m_graceful_termination_timeout = p_timeout; }

		auto set_environment(const process_detail::env_map_t& p_env) -> void { m_environment = p_env; }

		auto set_environment_variable(const std::string& p_key, const std::string& p_value) -> void { m_environment[p_key] = p_value; }

		auto set_capture_output(bool p_capture) -> void { m_capture_output = p_capture; }

		auto set_stdout_callback(output_callback_t p_callback) -> void { m_stdout_callback = std::move(p_callback); }

		auto set_stderr_callback(output_callback_t p_callback) -> void { m_stderr_callback = std::move(p_callback); }

		// =============================================================================
		// PUBLIC STATUS METHODS
		// =============================================================================

		auto is_running() const -> bool
		{
			auto current_status = m_status.load();
			auto current_pid	= m_pid.load();
			return current_status == status_t::running && current_pid > 0 && kill(current_pid, 0) == 0;
		}

		auto is_finished() const -> bool
		{
			if (m_future.valid())
			{
				return m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
			}
			auto current_status = m_status.load();
			return current_status != status_t::running;
		}

		// =============================================================================
		// PUBLIC EXECUTION METHODS
		// =============================================================================

		auto execute(const std::string& p_command) -> void { execute_synchronous(p_command); }

		auto execute_async(const std::string& p_command) -> void
		{
			{
				const std::lock_guard<std::mutex> lock(*m_pid_mutex);
				m_pid_ready.store(false);
			}

			reset_termination_request();
			m_future = std::async(std::launch::async, [this, p_command]() { execute_synchronous(p_command); });
			wait_for_pid();
		}

		auto wait_for_pid(std::chrono::milliseconds p_timeout = process_consts::k_pid_wait_timeout_ms) -> bool
		{
			std::unique_lock<std::mutex> lock(*m_pid_mutex);
			return m_pid_condition->wait_for(lock, p_timeout, [this] { return m_pid_ready.load(); });
		}

		auto wait() -> void
		{
			if (m_future.valid() && (is_running() && !is_finished()))
			{
				m_future.wait();
			}
		}

		auto wait_for(std::chrono::milliseconds p_timeout) -> bool
		{
			if (m_future.valid() && (is_running() && !is_finished()))
			{
				return m_future.wait_for(p_timeout) == std::future_status::ready;
			}
			return true;
		}

		auto terminate(bool p_force = false) -> bool
		{
			auto current_pid = m_pid.load();
			if (current_pid <= 0 || !is_running())
			{
				return true;
			}

			request_termination(p_force);
			return wait_for_termination_completion();
		}

		// =============================================================================
		// STATIC UTILITY METHODS
		// =============================================================================

		static auto check_process_running(const std::string& p_process_name) -> bool
		{
			return process_detail::process_utilities::check_process_running(p_process_name);
		}

	private:
		// =============================================================================
		// PRIVATE UTILITY METHODS
		// =============================================================================

		auto set_error(error_code_t p_error) -> void { m_last_error.store(p_error); }

		static auto error_code_to_string(error_code_t p_error) -> std::string
		{
			switch (p_error)
			{
			case error_code_t::none:
				return "No error";
			case error_code_t::fork_failed:
				return "Fork operation failed";
			case error_code_t::pipe_creation_failed:
				return "Pipe creation failed";
			case error_code_t::exec_failed:
				return "Process execution failed";
			case error_code_t::working_dir_failed:
				return "Working directory change failed";
			case error_code_t::env_setup_failed:
				return "Environment setup failed";
			case error_code_t::stdin_write_failed:
				return "Stdin write operation failed";
			case error_code_t::termination_failed:
				return "Process termination failed";
			case error_code_t::timeout_exceeded:
				return "Process timeout exceeded";
			case error_code_t::invalid_state:
				return "Invalid process state";
			case error_code_t::system_error:
				return "System error occurred";
			case error_code_t::insufficient_privileges:
				return "Insufficient privileges for operation";
			case error_code_t::resource_limit_exceeded:
				return "Resource limit exceeded";
			default:
				return "Unknown error";
			}
		}

		// =============================================================================
		// TERMINATION HANDLING
		// =============================================================================

		auto reset_termination_request() -> void
		{
			m_termination_request.requested.store(false);
			m_termination_request.force.store(false);
			m_termination_request.completed.store(false);
		}

		auto request_termination(bool p_force) -> void
		{
			if (!m_termination_request.requested.exchange(true))
			{
				m_termination_request.request_time = std::chrono::steady_clock::now();
			}

			if (p_force)
			{
				m_termination_request.force.store(true);
			}
		}

		auto wait_for_termination_completion() const -> bool
		{
			auto start_time = std::chrono::steady_clock::now();

			while (std::chrono::steady_clock::now() - start_time < process_consts::k_termination_timeout_ms)
			{
				if (m_termination_request.completed.load())
				{
					return true;
				}
				std::this_thread::sleep_for(process_consts::k_termination_check_ms);
			}
			return false;
		}

		auto terminate_sync(bool p_force) -> void
		{
			auto current_pid = m_pid.load();
			if (current_pid <= 0 || !is_running())
			{
				return;
			}

			request_termination(p_force);
			wait_for_termination_completion();
		}

		auto wait_with_timeout() -> void
		{
			if (!m_future.valid())
			{
				return;
			}

			if (m_future.wait_for(process_consts::k_termination_timeout_ms) != std::future_status::ready)
			{
				request_termination(true);

				if (m_future.wait_for(process_consts::k_future_wait_timeout_ms) != std::future_status::ready)
				{
					future_waiter waiter(std::move(m_future));
					std::thread(std::move(waiter)).detach();
				}
			}
		}

		auto handle_termination_request() -> bool
		{
			if (!m_termination_request.requested.load())
			{
				return false;
			}

			const bool force_termination = m_termination_request.force.load();
			const bool success			 = perform_termination(force_termination);

			m_termination_request.completed.store(true);
			return success;
		}

		auto perform_termination(bool p_force) -> bool
		{
			auto current_pid = m_pid.load();
			if (current_pid <= 0)
			{
				return true;
			}

			bool use_forced_termination = p_force;
			bool sent_signal			= false;

			for (std::int32_t attempt = 0; attempt < 2; ++attempt)
			{
				const std::int32_t signal = use_forced_termination ? SIGKILL : SIGTERM;
				sent_signal				  = false;

				if (killpg(current_pid, signal) == 0 || kill(-current_pid, signal) == 0 || kill(current_pid, signal) == 0)
				{
					sent_signal = true;
				}

				if (sent_signal)
				{
					if (wait_signal(current_pid, use_forced_termination))
					{
						return true;
					}

					if (!use_forced_termination && !p_force)
					{
						use_forced_termination = true;
						continue;
					}
				}
				else
				{
					set_error(error_code_t::termination_failed);
				}

				break;
			}

			m_status.store(status_t::terminated);
			return sent_signal;
		}

		auto wait_signal(pid_t p_pid, bool p_force) -> bool
		{
			auto start_time		  = std::chrono::steady_clock::now();
			auto timeout_duration = p_force ? process_consts::k_termination_timeout_ms : m_graceful_termination_timeout;

			while (std::chrono::steady_clock::now() - start_time < timeout_duration)
			{
				std::int32_t status = 0;
				const pid_t result	= waitpid(-p_pid, &status, WNOHANG);

				if (result > 0)
				{
					m_return_code = WEXITSTATUS(status);
					m_status.store(status_t::terminated);
					return true;
				}
				if (result == -1 && errno == ECHILD)
				{
					m_status.store(status_t::terminated);
					return true;
				}

				std::this_thread::sleep_for(process_consts::k_terminate_retry_sleep_ms);
			}

			return false;
		}

		// =============================================================================
		// CORE EXECUTION LOGIC
		// =============================================================================

		auto execute_synchronous(const std::string& p_command) -> void
		{
			m_status.store(status_t::running);
			set_error(error_code_t::none);

			{
				const std::lock_guard<std::mutex> lock(*m_output_mutex);
				m_stdout_buffer.clear();
				m_stderr_buffer.clear();
				m_command = p_command;
			}

			auto pipe_setup_result = setup_pipes();
			if (!pipe_setup_result.is_valid)
			{
				set_error(error_code_t::pipe_creation_failed);
				m_status.store(status_t::error);
				return;
			}

			const pid_t child_pid = fork();

			if (child_pid == -1)
			{
				set_error(error_code_t::fork_failed);
				m_status.store(status_t::error);
				return;
			}

			if (child_pid == 0)
			{
				setup_child_process(p_command, pipe_setup_result);
				_exit(process_consts::k_exec_error_code);
			}

			m_pid.store(child_pid);

			{
				const std::lock_guard<std::mutex> lock(*m_pid_mutex);
				m_pid_ready.store(true);
			}
			m_pid_condition->notify_all();

			handle_parent_process(pipe_setup_result);
		}

		// =============================================================================
		// PIPE MANAGEMENT
		// =============================================================================

		struct pipe_setup_t
		{
			process_detail::pipe_wrapper stdout_pipe;
			process_detail::pipe_wrapper stderr_pipe;
			process_detail::pipe_wrapper stdin_pipe;
			bool need_stdout{};
			bool need_stderr{};
			bool need_stdin{};
			bool is_valid{};
		};

		auto setup_pipes() -> pipe_setup_t
		{
			pipe_setup_t result;
			result.need_stdout = should_use_stdout_pipe();
			result.need_stderr = should_use_stderr_pipe();
			result.need_stdin  = !m_stdin_from_file;

			if (result.need_stdout && !result.stdout_pipe.is_valid())
			{
				return result;
			}
			if (result.need_stderr && !result.stderr_pipe.is_valid())
			{
				return result;
			}
			if (result.need_stdin && !result.stdin_pipe.is_valid())
			{
				return result;
			}

			result.is_valid = true;
			return result;
		}

		auto should_use_stdout_pipe() const -> bool
		{
			return (m_stdout_redirect_mode == redirect_mode_t::memory || m_stdout_redirect_mode == redirect_mode_t::both || m_stdout_callback);
		}

		auto should_use_stderr_pipe() const -> bool
		{
			return (m_stderr_redirect_mode == redirect_mode_t::memory || m_stderr_redirect_mode == redirect_mode_t::both || m_stderr_callback);
		}

		// =============================================================================
		// CHILD PROCESS SETUP
		// =============================================================================

		auto setup_child_process(const std::string& p_command, pipe_setup_t& p_pipes) -> void
		{
			setup_child_io(p_pipes);
			close_child_pipe_ends(p_pipes);

			if (!m_working_directory.empty())
			{
				if (chdir(m_working_directory.c_str()) != 0)
				{
					_exit(process_consts::k_exec_error_code);
				}
			}

			execute_command(p_command);
		}

		auto setup_child_io(pipe_setup_t& p_pipes) -> void
		{
			setup_child_stdin(p_pipes);
			setup_child_stdout(p_pipes);
			setup_child_stderr(p_pipes);
		}

		auto setup_child_stdin(pipe_setup_t& p_pipes) -> void
		{
			if (m_stdin_from_file && !m_stdin_file_path.empty())
			{
				const std::int32_t stdin_descriptor = open(m_stdin_file_path.c_str(), O_RDONLY);
				if (stdin_descriptor != -1)
				{
					if (dup2(stdin_descriptor, STDIN_FILENO) == -1)
					{
						close(stdin_descriptor);
						_exit(process_consts::k_exec_error_code);
					}
					close(stdin_descriptor);
				}
			}
			else if (p_pipes.need_stdin)
			{
				if (dup2(p_pipes.stdin_pipe.read_fd(), STDIN_FILENO) == -1)
				{
					_exit(process_consts::k_exec_error_code);
				}
			}
		}

		auto setup_child_stdout(pipe_setup_t& p_pipes) -> void
		{
			if (m_stdout_redirect_mode == redirect_mode_t::file && !m_stdout_file_path.empty())
			{
				const std::int32_t stdout_descriptor =
					open(m_stdout_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, process_consts::k_file_permissions);
				if (stdout_descriptor != -1)
				{
					if (dup2(stdout_descriptor, STDOUT_FILENO) == -1)
					{
						close(stdout_descriptor);
						_exit(process_consts::k_exec_error_code);
					}
					close(stdout_descriptor);
				}
			}
			else if (p_pipes.need_stdout)
			{
				if (dup2(p_pipes.stdout_pipe.write_fd(), STDOUT_FILENO) == -1)
				{
					_exit(process_consts::k_exec_error_code);
				}
			}
		}

		auto setup_child_stderr(pipe_setup_t& p_pipes) -> void
		{
			if (m_stderr_redirect_mode == redirect_mode_t::file && !m_stderr_file_path.empty())
			{
				const std::int32_t stderr_descriptor =
					open(m_stderr_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, process_consts::k_file_permissions);
				if (stderr_descriptor != -1)
				{
					if (dup2(stderr_descriptor, STDERR_FILENO) == -1)
					{
						close(stderr_descriptor);
						_exit(process_consts::k_exec_error_code);
					}
					close(stderr_descriptor);
				}
			}
			else if (p_pipes.need_stderr)
			{
				if (dup2(p_pipes.stderr_pipe.write_fd(), STDERR_FILENO) == -1)
				{
					_exit(process_consts::k_exec_error_code);
				}
			}
		}

		static auto close_child_pipe_ends(pipe_setup_t& p_pipes) -> void
		{
			if (p_pipes.need_stdout)
			{
				p_pipes.stdout_pipe.close_read();
			}
			if (p_pipes.need_stderr)
			{
				p_pipes.stderr_pipe.close_read();
			}
			if (p_pipes.need_stdin)
			{
				p_pipes.stdin_pipe.close_write();
			}
		}

		auto execute_command(const std::string& p_command) -> void
		{
			if (setpgid(0, 0) != 0)
			{
				_exit(process_consts::k_exec_error_code);
			}

			setup_environment_and_exec(p_command);
			_exit(process_consts::k_exec_error_code);
		}

		auto setup_environment_and_exec(const std::string& p_command) -> void
		{
			static thread_local std::array<char, 3> shell_name = {'s', 'h', '\0'};
			static thread_local std::array<char, 3> dash_c_arg = {'-', 'c', '\0'};
			std::vector<char> cmd_vector(p_command.begin(), p_command.end());
			cmd_vector.push_back('\0');
			std::array<char*, 4> argv = {shell_name.data(), dash_c_arg.data(), cmd_vector.data(), nullptr};

			std::vector<char*> environ_copy;
			std::vector<char*> new_environ;

			create_environment_variables(environ_copy, new_environ);

			execvpe("/bin/sh", argv.data(), environ_copy.data());

			cleanup_environment_variables(new_environ);
		}

		auto create_environment_variables(std::vector<char*>& p_environ_copy, std::vector<char*>& p_new_environ) -> void
		{
			for (auto* iter = environ; *iter != nullptr; iter = std::next(iter))
			{
				p_environ_copy.push_back(*iter);
			}

			for (const auto& env_iter : m_environment)
			{
				std::string env_var = env_iter.first + '=' + env_iter.second;
				char* env_str		= new char[env_var.size() + 1];
				std::copy(env_var.begin(), env_var.end(), env_str);
				char* last_char = std::next(env_str, static_cast<std::ptrdiff_t>(env_var.size()));
				*last_char		= '\0';
				p_new_environ.push_back(env_str);
			}

			p_environ_copy.insert(p_environ_copy.end(), p_new_environ.begin(), p_new_environ.end());
			p_environ_copy.erase(std::remove_if(p_environ_copy.begin(),
												p_environ_copy.end(),
												[](const char* p_env) -> bool { return nullptr == p_env || std::string(p_env).empty(); }),
								 p_environ_copy.end());
			p_environ_copy.push_back(nullptr);
		}

		static auto cleanup_environment_variables(const std::vector<char*>& p_new_environ) -> void
		{
			for (auto* env_str : p_new_environ)
			{
				delete[] env_str;
			}
		}

		// =============================================================================
		// PARENT PROCESS HANDLING
		// =============================================================================

		auto handle_parent_process(pipe_setup_t& p_pipes) -> void
		{
			close_parent_pipe_ends(p_pipes);
			write_stdin_data(p_pipes);
			run_main_loop(p_pipes);
			handle_both_mode_files();
			trim_newlines();
		}

		static auto close_parent_pipe_ends(pipe_setup_t& p_pipes) -> void
		{
			if (p_pipes.need_stdout)
			{
				p_pipes.stdout_pipe.close_write();
			}
			if (p_pipes.need_stderr)
			{
				p_pipes.stderr_pipe.close_write();
			}
			if (p_pipes.need_stdin)
			{
				p_pipes.stdin_pipe.close_read();
			}
		}

		auto write_stdin_data(pipe_setup_t& p_pipes) -> void
		{
			if (p_pipes.need_stdin && !m_stdin_buffer.empty())
			{
				std::size_t total_written		 = 0;
				const std::size_t total_to_write = m_stdin_buffer.length();
				const char* data_ptr			 = m_stdin_buffer.c_str();

				while (total_written < total_to_write)
				{
					const process_detail::span<const char> data_span(data_ptr,
																	 static_cast<process_detail::span<const char>::difference_type>(total_written),
																	 total_to_write - total_written);
					const ssize_t written = write(p_pipes.stdin_pipe.write_fd(), data_span.data(), total_to_write - total_written);

					if (written == -1)
					{
						if (errno == EAGAIN || errno == EWOULDBLOCK)
						{
							std::this_thread::sleep_for(process_consts::k_stdin_write_retry_ms);
							continue;
						}
						if (errno == EPIPE)
						{
							break;
						}
						set_error(error_code_t::stdin_write_failed);
						break;
					}
					if (written == 0)
					{
						break;
					}
					total_written += static_cast<std::size_t>(written);
				}
			}

			if (p_pipes.need_stdin)
			{
				p_pipes.stdin_pipe.close_write();
			}
		}

		auto run_main_loop(pipe_setup_t& p_pipes) -> void
		{
			auto start_time			 = std::chrono::steady_clock::now();
			std::int32_t idle_cycles = 0;

			auto last_termination_check = std::chrono::steady_clock::now();

			while (true)
			{
				auto now = std::chrono::steady_clock::now();
				if (now - last_termination_check >= process_consts::k_termination_check_ms)
				{
					if (handle_termination_request())
					{
						break;
					}
					last_termination_check = now;
				}

				if (check_timeout(start_time))
				{
					break;
				}

				const bool had_activity = handle_pipe_activity(p_pipes);
				if (had_activity)
				{
					idle_cycles = 0;
				}

				if (check_process_status())
				{
					read_remaining_pipe_data(p_pipes);
					break;
				}

				if (!had_activity)
				{
					idle_cycles++;
					if (idle_cycles > process_consts::k_max_idle_cycles || (!p_pipes.need_stdout && !p_pipes.need_stderr))
					{
						std::this_thread::sleep_for(process_consts::k_main_loop_sleep_ms);
					}
				}
			}
		}

		auto handle_pipe_activity(pipe_setup_t& p_pipes) -> bool
		{
			const bool has_active_pipes = p_pipes.need_stdout || p_pipes.need_stderr;

			if (!has_active_pipes)
			{
				return false;
			}

			fd_set read_fds;
			FD_ZERO(&read_fds);

			std::int32_t max_fd = 0;
			if (p_pipes.need_stdout)
			{
				FD_SET(p_pipes.stdout_pipe.read_fd(), &read_fds);
				max_fd = std::max(max_fd, p_pipes.stdout_pipe.read_fd());
			}
			if (p_pipes.need_stderr)
			{
				FD_SET(p_pipes.stderr_pipe.read_fd(), &read_fds);
				max_fd = std::max(max_fd, p_pipes.stderr_pipe.read_fd());
			}

			struct timeval timeout		= {0, static_cast<__suseconds_t>(process_consts::k_select_timeout_us.count())};
			const std::int32_t activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);

			if (activity > 0)
			{
				handle_pipe_output(p_pipes, read_fds);
				return true;
			}

			if (activity < 0 && errno == EINTR)
			{
				return false;
			}

			return false;
		}

		auto handle_pipe_output(pipe_setup_t& p_pipes, fd_set& p_read_fds) -> void
		{
			if (p_pipes.need_stdout && FD_ISSET(p_pipes.stdout_pipe.read_fd(), &p_read_fds))
			{
				auto buffer				 = process_detail::buffer_pool::instance().acquire();
				const ssize_t bytes_read = read(p_pipes.stdout_pipe.read_fd(), buffer->data(), buffer->size() - 1);
				if (bytes_read > 0)
				{
					process_stdout_data(*buffer, static_cast<std::size_t>(bytes_read));
				}
				process_detail::buffer_pool::instance().release(std::move(buffer));
			}

			if (p_pipes.need_stderr && FD_ISSET(p_pipes.stderr_pipe.read_fd(), &p_read_fds))
			{
				auto buffer				 = process_detail::buffer_pool::instance().acquire();
				const ssize_t bytes_read = read(p_pipes.stderr_pipe.read_fd(), buffer->data(), buffer->size() - 1);
				if (bytes_read > 0)
				{
					process_stderr_data(*buffer, static_cast<std::size_t>(bytes_read));
				}
				process_detail::buffer_pool::instance().release(std::move(buffer));
			}
		}

		auto read_remaining_pipe_data(pipe_setup_t& p_pipes) -> void
		{
			for (std::int32_t attempt = 0; attempt < process_consts::k_max_drain_attempts; ++attempt)
			{
				bool read_data = false;

				if (p_pipes.need_stdout)
				{
					auto buffer				 = process_detail::buffer_pool::instance().acquire();
					const ssize_t bytes_read = read(p_pipes.stdout_pipe.read_fd(), buffer->data(), buffer->size() - 1);
					if (bytes_read > 0)
					{
						process_stdout_data(*buffer, static_cast<std::size_t>(bytes_read));
						read_data = true;
					}
					process_detail::buffer_pool::instance().release(std::move(buffer));
				}

				if (p_pipes.need_stderr)
				{
					auto buffer				 = process_detail::buffer_pool::instance().acquire();
					const ssize_t bytes_read = read(p_pipes.stderr_pipe.read_fd(), buffer->data(), buffer->size() - 1);
					if (bytes_read > 0)
					{
						process_stderr_data(*buffer, static_cast<std::size_t>(bytes_read));
						read_data = true;
					}
					process_detail::buffer_pool::instance().release(std::move(buffer));
				}

				if (!read_data)
				{
					break;
				}
			}
		}

		auto check_timeout(std::chrono::steady_clock::time_point p_start_time) -> bool
		{
			if (m_timeout.count() > 0)
			{
				auto elapsed = std::chrono::steady_clock::now() - p_start_time;
				if (elapsed >= m_timeout)
				{
					set_error(error_code_t::timeout_exceeded);
					request_termination(true);
					handle_termination_request();
					m_status.store(status_t::timeout);
					return true;
				}
			}
			return false;
		}

		auto check_process_status() -> bool
		{
			auto current_pid = m_pid.load();
			if (current_pid <= 0)
			{
				return false;
			}

			std::int32_t status = 0;
			const pid_t result	= waitpid(current_pid, &status, WNOHANG);

			if (result == current_pid)
			{
				m_return_code = WEXITSTATUS(status);
				m_status.store(status_t::finished);
				return true;
			}
			if (result == -1)
			{
				set_error(error_code_t::system_error);
				m_status.store(status_t::error);
				return true;
			}
			return false;
		}

		auto handle_both_mode_files() -> void
		{
			if (m_stdout_redirect_mode == redirect_mode_t::both && !m_stdout_file_path.empty())
			{
				std::string file_content;
				if (process_detail::process_utilities::read_file_content(m_stdout_file_path, file_content))
				{
					const std::lock_guard<std::mutex> lock(*m_output_mutex);
					m_stdout_buffer = std::move(file_content);
				}
			}

			if (m_stderr_redirect_mode == redirect_mode_t::both && !m_stderr_file_path.empty())
			{
				std::string file_content;
				if (process_detail::process_utilities::read_file_content(m_stderr_file_path, file_content))
				{
					const std::lock_guard<std::mutex> lock(*m_output_mutex);
					m_stderr_buffer = std::move(file_content);
				}
			}
		}

		auto process_stdout_data(process_detail::data_t& p_buffer, std::size_t p_bytes_read) -> void
		{
			if (p_bytes_read < p_buffer.size())
			{
				p_buffer.at(p_bytes_read) = '\0';
			}
			const std::string output(p_buffer.data());

			const bool should_capture =
				m_capture_output && (m_stdout_redirect_mode == redirect_mode_t::memory || m_stdout_redirect_mode == redirect_mode_t::both);
			if (should_capture)
			{
				const std::lock_guard<std::mutex> lock(*m_output_mutex);
				m_stdout_buffer += output;
			}

			if (m_stdout_callback)
			{
				m_stdout_callback(output);
			}
		}

		auto process_stderr_data(process_detail::data_t& p_buffer, std::size_t p_bytes_read) -> void
		{
			if (p_bytes_read < p_buffer.size())
			{
				p_buffer.at(p_bytes_read) = '\0';
			}
			const std::string output(p_buffer.data());

			const bool should_capture =
				m_capture_output && (m_stderr_redirect_mode == redirect_mode_t::memory || m_stderr_redirect_mode == redirect_mode_t::both);
			if (should_capture)
			{
				const std::lock_guard<std::mutex> lock(*m_output_mutex);
				m_stderr_buffer += output;
			}

			if (m_stderr_callback)
			{
				m_stderr_callback(output);
			}
		}

		auto trim_newlines() -> void
		{
			const std::lock_guard<std::mutex> lock(*m_output_mutex);

			if (!m_stdout_buffer.empty() && m_stdout_buffer.back() == '\n')
			{
				m_stdout_buffer.pop_back();
			}

			if (!m_stderr_buffer.empty() && m_stderr_buffer.back() == '\n')
			{
				m_stderr_buffer.pop_back();
			}
		}
	};
}	 // namespace utils
#endif	  // PROCESS_HPP