/**
 * @file communications.cpp
 * @author anonimi_angels (gutsulyak.vladyslav2000@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-11-13
 *
 * @copyright
 *The MIT License (MIT)
 *Copyright (c) 2024 anonimi_angels
 *Permission is hereby granted, free of charge, to any person obtaining a copy
 *of this software and associated documentation files (the "Software"), to deal
 *in the Software without restriction, including without limitation the rights
 *to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *copies of the Software, and to permit persons to whom the Software is
 *furnished to do so, subject to the following conditions:
 *The above copyright notice and this permission notice shall be included in all
 *copies or substantial portions of the Software.
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *SOFTWARE.
 *
 */

// Include own header
#include "comms.hpp"

// Include other headers
#include "timer.hpp"
#include "proccess.hpp"

// Include linux headers
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

// Include c++ headers
#include <stdexcept>
#include <fstream>

#ifndef NDEBUG

#ifdef __linux__
#define BREAKPOINT __asm__("int $3")
#elif _WIN32
#define BREAKPOINT __debugbreak()
#endif

#define CATCH_EXCEPTION \
	do {                \
		BREAKPOINT;     \
	} while (0)

#else
#define CATCH_EXCEPTION
#endif

namespace utils
{

	auto ssh_process::parse_error_code() -> void
	{
		char* err_msg	= nullptr;
		int32_t err_len = 0;
		m_ssh_rc		= libssh2_session_last_error(m_ssh_session, &err_msg, &err_len, 0);

		if (m_ssh_rc == LIBSSH2_ERROR_NONE || m_ssh_rc == LIBSSH2_ERROR_EAGAIN) { return; }

		// Retrieve errno stringified
		std::string errno_msg = strerror(errno);
		std::string err_out	  = std::format("\nLast errno: {}\nError:({}) {}", errno_msg, m_ssh_rc, err_msg);

		CATCH_EXCEPTION;
		throw std::runtime_error(err_out);
	}

	ssh_process::ssh_process()
	{
		m_logger = A_MAKE_UNIQUE_LOGGER;

		auto default_callback = [this](const std::string& in_msg) -> void { m_logger->error("{}", in_msg); };

		set_log_callback(default_callback);
	}

	auto ssh_process::pull_file(const std::string& path_local, const std::string& path_remote, const bool& check_sha) -> void
	{
		std::string normalized_local  = normalize_path(path_local);
		std::string normalized_remote = normalize_path(path_remote);

		request_sftp_file(normalized_local, normalized_remote);

		if (!check_sha) { return; }

		if (!ssh_process::check_sha(normalized_local, normalized_remote))
		{
			CATCH_EXCEPTION;
			throw std::runtime_error("Error: downloaded file and remote file SHA mismatch");
		}
	}

	auto ssh_process::push_file(const std::string& path_local, const std::string& path_remote, const bool& check_sha) -> void
	{
		std::string normalized_local  = normalize_path(path_local);
		std::string normalized_remote = normalize_path(path_remote);

		send_sftp_file(normalized_local, normalized_remote);

		if (!check_sha) { return; }

		if (!ssh_process::check_sha(normalized_local, normalized_remote))
		{
			CATCH_EXCEPTION;
			throw std::runtime_error("Error: uploaded file and remote file SHA mismatch");
		}
	}

	void ssh_process::open_ssh_connection()
	{
		m_ssh_rc = libssh2_init(0);
		if (m_ssh_rc != 0)
		{
			close_ssh_connection();
			throw std::runtime_error("Error: Unable to initialize libssh2");
		}

		m_logger->trace("Creating SSH session");
		m_ssh_session = libssh2_session_init();
		if (m_ssh_session == nullptr)
		{
			close_ssh_connection();
			throw std::runtime_error("Error: Unable to initialize SSH session");
		}

		// Setting non blocking mode
		libssh2_session_set_blocking(m_ssh_session, 0);

		m_logger->trace("Creating socket on host");
		m_ssh_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (m_ssh_socket == LIBSSH2_INVALID_SOCKET) { parse_error_code(); }

		m_ssh_hostaddr = inet_addr(m_ip.c_str());

		m_ssh_sin.sin_family	  = AF_INET;
		m_ssh_sin.sin_port		  = htons(22);
		m_ssh_sin.sin_addr.s_addr = m_ssh_hostaddr;

		m_logger->trace("Connecting to remote host");
		connect_to_rhost();

		m_logger->trace("Performing handshake");
		while_eagain([&]() { return libssh2_session_handshake(m_ssh_session, m_ssh_socket); });

		m_logger->trace("Authenticating user");
		while_eagain([&]() { return libssh2_userauth_password(m_ssh_session, m_user.c_str(), m_pass.c_str()); });

		if (m_ssh_rc != 0) { parse_error_code(); }

		set_channel_env();

		m_logger->trace("SSH connection established");
		m_state = true;
	}

	auto ssh_process::connect_to_rhost() -> void
	{

#define TO_STRING(x) #x

		int32_t prev_errno = 0;
		bool wait		   = false;

		while ((m_ssh_rc = connect(m_ssh_socket, (struct sockaddr*)&m_ssh_sin, sizeof(struct sockaddr_in))) == -1)
		{

			if (prev_errno != errno)
			{

				switch (errno)
				{
				case EINPROGRESS:
				{
					m_log_callback(std::format("{}", TO_STRING(EINPROGRESS)));
					wait = true;
				}
				break;
				case EAGAIN:
				{
					m_log_callback(std::format("{}", TO_STRING(EAGAIN)));
					wait = true;
				}
				break;
				case EHOSTUNREACH:
				{
					m_log_callback(std::format("{}", TO_STRING(EHOSTUNREACH)));
					wait = true;
				}
				break;
				default:
				{
					parse_error_code();
				}
				}

				prev_errno = errno;
			}

			if (wait) { wait_socket(); }
		}
	}

	auto ssh_process::set_channel_env() -> void
	{
		// Set environment variable if needed
		// libssh2_channel_setenv(m_ssh_channel, "DISPLAY", "0");
	}

	auto ssh_process::check_sha(const std::string& path_local, const std::string& path_remote) -> bool
	{
		std::string cmd;
		std::string local_sha;
		std::string remote_sha;

		std::filesystem::path fs_path_local(path_local);

		if (!std::filesystem::exists(fs_path_local)) { return false; }

		cmd = std::format("test -f {}", path_remote);
		exec(cmd);

		if (m_return_code != 0) { return false; }

		// Get local sha
		cmd		  = std::format("sha256sum {}", path_local);
		local_sha = proccess(cmd).get_output();
		local_sha = local_sha.substr(0, local_sha.find(' '));

		// Get remote sha
		cmd = std::format("sha256sum {}", path_remote);
		exec(cmd);
		remote_sha = m_buffer.substr(0, m_buffer.find(' '));

		return local_sha == remote_sha; // True if the sha is the same
	}

	void ssh_process::close_ssh_connection()
	{
		bool action_performed = false;

		if (m_sftp_session != nullptr)
		{
			libssh2_sftp_shutdown(m_sftp_session);
			m_sftp_session = nullptr;

			action_performed = true;
		}

		if (close_ssh_channel()) { action_performed = true; }

		if (m_ssh_known_hosts != nullptr)
		{
			libssh2_knownhost_free(m_ssh_known_hosts);
			m_ssh_known_hosts = nullptr;

			action_performed = true;
		}

		if (m_ssh_session != nullptr)
		{
			libssh2_session_disconnect(m_ssh_session, "Normal Shutdown");
			libssh2_session_free(m_ssh_session);
			m_ssh_session = nullptr;

			action_performed = true;
		}

		if (m_ssh_socket != LIBSSH2_INVALID_SOCKET)
		{
			close(m_ssh_socket);
			m_ssh_socket = LIBSSH2_INVALID_SOCKET;

			action_performed = true;
		}

		libssh2_exit();

		if (action_performed) { m_logger->trace("SSH connection closed"); }

		m_state = false;
	}

	void ssh_process::exec(const std::string& cmd)
	{
		send_ssh_command(cmd);
	}

	void ssh_process::send_ssh_command(const std::string& cmd)
	{
		open_ssh_channel();

		m_logger->debug("Executing command: {}", cmd);
		while_eagain([&]() { return libssh2_channel_exec(m_ssh_channel, cmd.c_str()); });

		m_buffer.clear();
		m_err_buffer.clear();

		ssize_t nread = 0;
		std::array<char, 0x4000> buffer{};

		while (true)
		{
			nread = while_eagain([&]() { return libssh2_channel_read(m_ssh_channel, buffer.data(), buffer.size()); });

			if (nread > 0) { m_buffer.append(buffer.data(), static_cast<std::size_t>(nread)); }
			else { break; }
		}

		buffer = decltype(buffer){};

		// Read error buff
		while (true)
		{
			nread = while_eagain([&]() { return libssh2_channel_read_stderr(m_ssh_channel, buffer.data(), buffer.size()); });

			if (nread > 0) { m_err_buffer.append(buffer.data(), static_cast<std::size_t>(nread)); }
			else { break; }
		}

		// If ending with \n remove it
		if (!m_buffer.empty() && m_buffer.back() == '\n') { m_buffer.pop_back(); }
		if (!m_err_buffer.empty() && m_err_buffer.back() == '\n') { m_err_buffer.pop_back(); }

		m_logger->debug("Command rc: {}, buff: {}, err_buff: {}", m_return_code, m_buffer, m_err_buffer);

		close_ssh_channel();
	}

	void ssh_process::request_sftp_file(const std::string& path_local, const std::string& path_remote)
	{
		// If files are the same, do nothing
		if (check_sha(path_local, path_remote))
		{
			m_logger->trace("No ops: {} and {} are the same", path_local, path_remote);
			return;
		}

		open_sftp_session();

		m_sftp_file_info = {.flags = 0, .filesize = 0, .uid = 0, .gid = 0, .permissions = 0, .atime = 0, .mtime = 0};
		while_eagain([&]() { return libssh2_sftp_stat(m_sftp_session, path_remote.c_str(), &m_sftp_file_info); });

		if (m_ssh_rc != 0) { parse_error_code(); }

		open_sftp_handle_read(path_remote);

		// Read file
		const size_t file_size = m_sftp_file_info.filesize;
		size_t spin			   = 0;
		size_t total_read	   = 0;

		std::filesystem::path local_path_p(path_local);
		std::string local_parent_path = local_path_p.parent_path().string();

		m_logger->trace("Receiving file: {} ({} bytes), block size {}", path_remote, file_size, m_sftp_block_size);

		// Check if the local path exists
		if (!std::filesystem::exists(local_parent_path)) { std::filesystem::create_directories(local_parent_path); }

		// Open file for writing
		std::ofstream file(path_local, std::ios::binary);
		if (!file.is_open())
		{
			CATCH_EXCEPTION;
			throw std::runtime_error("Error: Unable to open file for writing");
		}

		precision_timer<std::chrono::milliseconds> timer;
		timer.restart();

		// char mem[m_sftp_block_size];
		std::vector<char> mem(m_sftp_block_size);
		ssize_t nread;

		while (true)
		{
			mem = std::vector<char>(m_sftp_block_size);

			const size_t to_read = std::clamp<size_t>(file_size - total_read, 0, m_sftp_block_size);
			nread				 = while_eagain(
				   [&]()
				   {
					   ++spin;
					   return libssh2_sftp_read(m_sftp_handle, mem.data(), to_read);
				   });

			if (nread > 0)
			{
				file.write(mem.data(), nread);

				total_read += nread;
			}
			else { break; }
		}

		timer.stop();

		if (spdlog::get_level() >= spdlog::level::trace)
		{
			size_t timer_elapsed = timer.get_elapsed().count();
			size_t k_bytes_sec	 = (file_size / 1024) / (timer_elapsed == 0 ? 1 : timer_elapsed);

			std::string out_put = std::format("Sent {} byte(s) in {} ms = {} K-bytes/sec spin: {}", total_read, timer_elapsed, k_bytes_sec, spin);

			m_logger->trace("{}", out_put);
		}

		file.close();

		close_sftp_handle();
	}

	void ssh_process::send_sftp_file(const std::string& path_local, const std::string& path_remote)
	{
		// If files are the same, do nothing
		if (check_sha(path_local, path_remote))
		{
			m_logger->trace("No ops: {} and {} are the same", path_local, path_remote);
			return;
		}

		open_sftp_session();

		open_sftp_handle_write(path_remote);

		// Open file for reading
		std::ifstream file(path_local, std::ios::binary);
		if (!file.is_open())
		{
			CATCH_EXCEPTION;
			throw std::runtime_error("Error: Unable to open file for reading");
		}

		const size_t file_size = std::filesystem::file_size(path_local);
		size_t spin			   = 0;

		m_logger->trace("Sending file: {} ({} bytes), block size {}", path_local, file_size, m_sftp_block_size);

		precision_timer<std::chrono::milliseconds> timer;
		timer.restart();

		ssize_t total_sent = 0;
		ssize_t nread;
		std::vector<char> mem(m_sftp_block_size);

		while (true)
		{
			mem = std::vector<char>(m_sftp_block_size);

			const size_t to_read = std::clamp<size_t>(file_size - total_sent, 0, m_sftp_block_size);
			// Read from position total_sent to total_sent + to_read
			file.seekg(total_sent, std::ios::beg);
			file.read(mem.data(), static_cast<int32_t>(to_read));

			nread = while_eagain(
				[&]()
				{
					++spin;
					return libssh2_sftp_write(m_sftp_handle, mem.data(), to_read);
				});

			if (nread > 0) { total_sent += nread; }
			else { break; }
		}

		timer.stop();

		if (spdlog::get_level() >= spdlog::level::trace)
		{
			size_t timer_elapsed = timer.get_elapsed().count();
			size_t k_bytes_sec	 = (file_size / 1024) / (timer_elapsed == 0 ? 1 : timer_elapsed);

			std::string out_put = std::format("Sent {} byte(s) in {} ms = {} K-bytes/sec spin: {}", total_sent, timer_elapsed, k_bytes_sec, spin);

			m_logger->trace("{}", out_put);
		}

		file.close();
		close_sftp_handle();
	}

	void ssh_process::open_ssh_channel()
	{
		m_logger->trace("Opening SSH channel");

		m_ssh_channel = while_last_err_eagain<decltype(m_ssh_channel)>([&]() { return libssh2_channel_open_session(m_ssh_session); });
	}

	auto ssh_process::close_ssh_channel() -> bool
	{
		if (m_ssh_channel != nullptr)
		{
			m_logger->trace("Closing SSH channel");
			while_eagain([&]() { return libssh2_channel_send_eof(m_ssh_channel); });
			while_eagain([&]() { return libssh2_channel_wait_eof(m_ssh_channel); });
			while_eagain([&]() { return libssh2_channel_eof(m_ssh_channel); });
			while_eagain([&]() { return libssh2_channel_close(m_ssh_channel); });
			while_eagain([&]() { return libssh2_channel_wait_closed(m_ssh_channel); });
			m_return_code = libssh2_channel_get_exit_status(m_ssh_channel);
			while_eagain([&]() { return libssh2_channel_free(m_ssh_channel); });

			m_ssh_channel = nullptr;
			parse_error_code();

			return true;
		}

		return false;
	}

	void ssh_process::open_sftp_session()
	{

		if (m_sftp_session != nullptr)
		{
			m_logger->trace("Closing previous SFTP session");

			libssh2_sftp_shutdown(m_sftp_session);
			m_sftp_session = nullptr;
		}

		m_logger->trace("Opening SFTP session");

		m_sftp_session = while_last_err_eagain<decltype(m_sftp_session)>([&]() { return libssh2_sftp_init(m_ssh_session); });
	}

	void ssh_process::open_sftp_handle_read(const std::string& path)
	{
		open_sftp_handle(path, LIBSSH2_FXF_READ, 0);
	}

	void ssh_process::open_sftp_handle_write(const std::string& path)
	{
		// open_sftp_handle(path, LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC, LIBSSH2_SFTP_S_IRUSR |
		// LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IXUSR);
		open_sftp_handle(path, LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC, 0777);
	}

	void ssh_process::open_sftp_handle(const std::string& path, const int32_t& flags, const int32_t& mode)
	{
		/*
			Flags:
				LIBSSH2_FXF_WRITE - Open the file for writing.
				LIBSSH2_FXF_CREAT - Create the file if it doesn't exist.
				LIBSSH2_FXF_TRUNC - Truncate the file to zero length if it exists.
				LIBSSH2_FXF_EXCL - Ensure that this call creates the file: if the file already exists, the call fails.

			Mode:
				LIBSSH2_SFTP_S_IRUSR - User has read permission.
				LIBSSH2_SFTP_S_IWUSR - User has write permission.
				LIBSSH2_SFTP_S_IXUSR - User has execute permission.
				LIBSSH2_SFTP_S_IRGRP - Group has read permission.
				LIBSSH2_SFTP_S_IWGRP - Group has write permission.
				LIBSSH2_SFTP_S_IXGRP - Group has execute permission.
				LIBSSH2_SFTP_S_IROTH - Others have read permission.
				LIBSSH2_SFTP_S_IWOTH - Others have write permission.
				LIBSSH2_SFTP_S_IXOTH - Others have execute permission.
		*/

		m_sftp_handle = while_last_err_eagain<decltype(m_sftp_handle)>([&]() { return libssh2_sftp_open(m_sftp_session, path.c_str(), flags, mode); });
	}

	void ssh_process::close_sftp_handle()
	{
		if (m_sftp_handle != nullptr)
		{
			libssh2_sftp_close(m_sftp_handle);
			m_sftp_handle = nullptr;
		}
	}

	auto ssh_process::get_bash_env(const std::string& env) -> std::string
	{
		std::string cmd;
		cmd = std::format("grep {} ~/.bashrc", env);

		exec(cmd);

		if (auto str_iter = m_buffer.find('='); str_iter != std::string::npos) { m_buffer = m_buffer.substr(str_iter + 1); }

		if (auto str_iter = m_buffer.find('\n'); str_iter != std::string::npos) { m_buffer = m_buffer.substr(0, str_iter); }

		if (m_ssh_rc != 0)
		{
			CATCH_EXCEPTION;
			throw std::runtime_error("Error: Unable to get bash environment variable");
		}

		return m_buffer;
	}

	auto ssh_process::get_env(const std::string& env) -> std::string
	{
		std::string cmd = std::format("echo ${}", env);

		exec(cmd);

		if (m_ssh_rc != 0)
		{
			CATCH_EXCEPTION;
			throw std::runtime_error("Error: Unable to get environment variable");
		}

		return m_buffer;
	}

	auto ssh_process::set_env(const std::string& env, const std::string& val) -> void
	{
		std::string cmd = std::format("echo export {}={} >> ~/.bashrc", env, val);

		exec(cmd);
	}

	auto ssh_process::wait_socket() -> int32_t
	{
		struct timeval timeout;
		fd_set fd_both	  = {};
		fd_set* fd_write  = nullptr;
		fd_set* fd_read	  = nullptr;
		int32_t direction = 0;

		timeout.tv_sec	= 10;
		timeout.tv_usec = 0;

		FD_ZERO(&fd_both);

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
		FD_SET(m_ssh_socket, &fd_both);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

		/* now make sure we wait in the correct direction */
		direction = libssh2_session_block_directions(m_ssh_session);

		if ((direction & LIBSSH2_SESSION_BLOCK_INBOUND) != 0) { fd_read = &fd_both; }

		if ((direction & LIBSSH2_SESSION_BLOCK_OUTBOUND) != 0) { fd_write = &fd_both; }

		return select(static_cast<int32_t>(m_ssh_socket + 1), fd_read, fd_write, nullptr, &timeout);
	}

	auto ssh_process::normalize_path(const std::string& path) -> std::string
	{
		return std::filesystem::path(path).lexically_normal().string();
	}

	auto ssh_process::move_to_dir(std::string_view&& in_path) -> void
	{
		const std::string cmd = std::format("cd {}", in_path);

		exec(cmd);

		if (m_ssh_rc != 0)
		{
			CATCH_EXCEPTION;
			throw std::runtime_error("Error: Unable to move to directory");
		}
	}

	auto ssh_process::check_existence(std::filesystem::path in_path, action_id in_action) -> bool
	{
		std::string l_str_path = in_path.string();
		std::string cmd;
		if (l_str_path.back() == '*') { l_str_path = l_str_path.substr(0, l_str_path.size() - 1); }

		in_path = normalize_path(l_str_path);

		switch (in_action)
		{
		case action_id::CHECK_FILE:
			cmd = std::format("test -f {}", in_path.string());
			break;
		case action_id::CHECK_DIR:
			cmd = std::format("test -d {}", in_path.string());
		case action_id::CREATE_DIR:
			cmd = std::format("test -d {}", in_path.string());
			break;
		}

		exec(cmd);

		switch (in_action)
		{
		case action_id::CHECK_FILE:
			return m_return_code == 0;
		case action_id::CHECK_DIR:
			return m_return_code == 0;
		case action_id::CREATE_DIR:
			if (m_return_code != 0)
			{
				cmd = std::format("mkdir -p {}", in_path.string());
				exec(cmd);

				return true;
			}
			break;
		}

		return m_return_code == 0;
	}

	auto ssh_process::while_eagain(std::function<std::int32_t()> in_func) -> std::int32_t
	{
		while (true)
		{
			m_ssh_rc = in_func();

			if (m_ssh_rc == LIBSSH2_ERROR_EAGAIN) { wait_socket(); }
			else if (m_ssh_rc < 0) { parse_error_code(); }
			else { break; }
		}

		return m_ssh_rc;
	}
} // namespace utils