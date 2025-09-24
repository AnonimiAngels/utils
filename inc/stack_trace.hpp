#pragma once
#ifndef STACK_TRACE_HPP
	#define STACK_TRACE_HPP

	// System includes
	#include <cstdint>
	#include <cstdlib>
	#include <cstring>
	#include <fstream>
	#include <iostream>
	#include <mutex>
	#include <sstream>
	#include <string>
	#include <utility>
	#include <vector>

	#include <inttypes.h>

	// Linux-specific includes for backtrace
	#ifdef __linux__
		#include <execinfo.h>
		#include <signal.h>
		#include <unistd.h>
	#endif

	// Convenience macro for pushing stack frames
	#define PUSH_STACK_TRACE(trace) trace.push_stack(__PRETTY_FUNCTION__, __LINE__, __FILE__)

namespace
{
	template <typename T> std::string to_string_helper(const T& v)
	{
		std::ostringstream os;
		os << v;
		return os.str();
	}

	template <typename T, typename... Args> void print_helper(std::ostream& os, const T& first, const Args&... rest)
	{
		os << first;
		/*  Recursively expand remaining arguments  */
		using expand = int[];
		(void)expand{0, (void(os << rest), 0)...};
	}
}	 // namespace

namespace global_utils
{

	class stack_trace
	{
	public:
		// Type aliases
		using self_t = stack_trace;

		// Stack frame structure
		struct stack_frame
		{
			std::string m_function;
			std::int32_t m_line;
			std::string m_file;

			stack_frame(const char* p_function, std::int32_t p_line, const char* p_file) : m_function(p_function), m_line(p_line), m_file(p_file) {}
		};

	private:
		// Member variables
		std::vector<stack_frame> m_stack;
		bool m_dump_on_destroy = false;

		// utils::process m_process;

	public:
		// Destructor
		~stack_trace()
		{
			if (m_dump_on_destroy && !m_stack.empty())
			{
				dump_stack();
			}
		}

		// Constructors (default, then parameterized in order of complexity)
		stack_trace() = default;

		stack_trace(bool p_dump_on_destroy) : m_dump_on_destroy(p_dump_on_destroy) {}

		// Deleted copy constructor and assignment operator
		stack_trace(const self_t&)				 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		// Move constructor and assignment operator
		stack_trace(self_t&& p_other) noexcept : m_stack(std::move(p_other.m_stack)), m_dump_on_destroy(p_other.m_dump_on_destroy)
		{
			p_other.m_dump_on_destroy = false;	  // Prevent double dump
		}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				m_stack					  = std::move(p_other.m_stack);
				m_dump_on_destroy		  = p_other.m_dump_on_destroy;
				p_other.m_dump_on_destroy = false;	  // Prevent double dump
			}
			return *this;
		}

	public:
		// Public methods
		auto push_stack(const char* p_function, std::int32_t p_line, const char* p_file) -> void { m_stack.emplace_back(p_function, p_line, p_file); }

		auto dump_stack() const -> void
		{
			if (m_stack.empty())
			{
				print_helper(std::cout, "Stack trace is empty\n");
				return;
			}

			print_helper(std::cout, "=== Stack Trace (", m_stack.size(), " frames) ===\n");

			std::size_t idx_for = 0;
			for (const auto& frame : m_stack)
			{
				std::string filename = frame.m_file;
				auto last_slash		 = filename.find_last_of("/\\");
				if (last_slash != std::string::npos)
					filename = filename.substr(last_slash + 1);

				print_helper(std::cout, "  #", idx_for++, ": ", frame.m_function, " at ", filename, ":", frame.m_line, '\n');
			}
			print_helper(std::cout, "=== End Stack Trace ===\n");
		}

		auto clear_stack() -> void { m_stack.clear(); }

		auto size() const -> std::size_t { return m_stack.size(); }

		auto empty() const -> bool { return m_stack.empty(); }

		auto gather_current_stack() -> void { gather_stack(); }

	private:
		// Private methods
		auto gather_stack() -> void
		{
	#ifdef __linux__
			constexpr std::int32_t max_frames = 64;
			void* buffer[max_frames];

			// Get the current stack trace
			std::int32_t frame_count = backtrace(buffer, max_frames);
			if (frame_count <= 0)
			{
				return;
			}

			// Skip the first frame (this function) and process the rest
			for (std::int32_t idx_for = 1; idx_for < frame_count; ++idx_for)
			{
				// Try to get detailed address information using addr2line
				auto address_info		  = get_address_info(buffer[idx_for]);
				std::string function_name = address_info.first;
				auto file_line_pair		  = address_info.second;
				std::string file_name	  = file_line_pair.first;
				std::int32_t line_number  = file_line_pair.second;

				// Attempt to demangle the function name
				std::string demangled_name = demangle_function_name(function_name);

				// If addr2line failed, fall back to backtrace_symbols
				if (function_name == "<unknown>" || file_name == "<unknown>")
				{
					char** symbols = backtrace_symbols(&buffer[idx_for], 1);
					if (symbols != nullptr)
					{
						std::string symbol_str(symbols[0]);
						std::string fallback_name = parse_symbol(symbol_str);

						// Use the better of the two results
						if (function_name == "<unknown>" && fallback_name != symbol_str)
						{
							demangled_name = demangle_function_name(fallback_name);
						}

						std::free(static_cast<void*>(symbols));
					}
				}

				// Ensure we have something meaningful to display
				if (demangled_name.empty() || demangled_name == "<unknown>")
				{
					demangled_name = "??";
				}
				if (file_name.empty() || file_name == "<unknown>")
				{
					file_name = "??";
				}

				m_stack.emplace_back(demangled_name.c_str(), line_number, file_name.c_str());
			}
	#endif
		}

		auto get_executable_path() -> std::string
		{
	#ifdef __linux__
			char exe_path[1024] = {0};
			ssize_t len			= readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
			if (len != -1)
			{
				exe_path[len] = '\0';
				return std::string(exe_path);
			}
	#endif
			return "";
		}

		auto format_address(void* p_address) -> std::string
		{
			std::ostringstream os;
			os << "0x" << std::hex << reinterpret_cast<uintptr_t>(p_address);
			return os.str();
		}

		auto get_address_info(void* p_address) -> std::pair<std::string, std::pair<std::string, std::int32_t> >
		{
			// Convert the address to uintptr_t
			uintptr_t address = reinterpret_cast<uintptr_t>(p_address);

			// Parse /proc/self/maps to find which module contains this address
			std::ifstream maps_file("/proc/self/maps");
			if (!maps_file.is_open())
			{
				return {"<unknown>", {"<unknown>", 0}};
			}

			uintptr_t base_address = 0;
			std::string module_path;
			std::string line;
			while (std::getline(maps_file, line))
			{
				std::istringstream iss(line);
				std::string address_range, perms, offset, dev, inode, pathname;

				// Parse the /proc/self/maps fields
				iss >> address_range >> perms >> offset >> dev >> inode;
				std::getline(iss, pathname);	// Get the rest of the line as the pathname

				// Clean up pathname - it may have leading spaces
				size_t first_non_space = pathname.find_first_not_of(" \t");
				if (first_non_space != std::string::npos)
				{
					pathname = pathname.substr(first_non_space);
				}

				// Skip non-file regions
				if (pathname.empty() || pathname[0] == '[' || pathname[0] == ' ')
				{
					continue;
				}

				uintptr_t start, end;
				if (sscanf(address_range.c_str(), "%" SCNxPTR "-%" SCNxPTR, &start, &end) != 2)
				{
					continue;
				}

				// Check if the address lies within this range
				if (address >= start && address < end)
				{
					base_address = start;
					module_path	 = pathname;
					break;
				}
			}

			maps_file.close();

			if (module_path.empty())
			{
				return {"<unknown>", {"<unknown>", 0}};
			}

			// Calculate the adjusted address relative to the base address of the module
			uintptr_t adjusted_address = address;	 // - base_address;
			(void)base_address;

			// Use addr2line with the module path and adjusted address

			std::ostringstream cmd;
			cmd << "addr2line -e " << module_path << " -f -C 0x" << std::hex << adjusted_address;

			std::string output;
			{
				FILE* pipe = popen(cmd.str().c_str(), "r");
				if (!pipe)
				{
					return {"<unknown>", {"<unknown>", 0}};
				}
				char buffer[128];
				while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
				{
					output += buffer;
				}
				pclose(pipe);
			}
			return parse_addr2line_output(output);
		}

		auto parse_addr2line_output(const std::string& p_output) -> std::pair<std::string, std::pair<std::string, std::int32_t> >
		{
			// Find the first newline to separate function name from file:line
			auto newline_pos = p_output.find('\n');
			if (newline_pos == std::string::npos)
			{
				return {"<unknown>", {"<unknown>", 0}};
			}

			std::string function_line = p_output.substr(0, newline_pos);
			std::string file_line	  = p_output.substr(newline_pos + 1);

			// Remove trailing newline from file_line if present
			if (!file_line.empty() && file_line.back() == '\n')
			{
				file_line.pop_back();
			}

			auto colon_pos = file_line.find_last_of(':');
			if (colon_pos == std::string::npos)
			{
				return {function_line, {"<unknown>", 0}};
			}

			std::string file_name = file_line.substr(0, colon_pos);
			std::string line_str  = file_line.substr(colon_pos + 1);

			std::int32_t line_number = 0;
			try
			{
				line_number = std::stoi(line_str);
			}
			catch (...)
			{
				line_number = 0;
			}

			// Extract just the filename from the full path
			auto last_slash = file_name.find_last_of("/\\");
			if (last_slash != std::string::npos)
			{
				file_name = file_name.substr(last_slash + 1);
			}

			return {function_line, {file_name, line_number}};
		}

		auto demangle_function_name(const std::string& p_function_name) -> std::string
		{
			// Check if the function name looks mangled (starts with _Z)
			if (p_function_name.empty() || p_function_name.substr(0, 2) != "_Z")
			{
				return p_function_name;
			}

			std::ostringstream cmd;
			cmd << "c++filt " << p_function_name;

			FILE* pipe = popen(cmd.str().c_str(), "r");
			if (!pipe)
			{
				return p_function_name;	   // Return original if popen fails
			}

			char buffer[256];
			std::string output;
			while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
			{
				output += buffer;
			}

			std::string demangled = output;

			// Remove trailing newline if present
			if (!demangled.empty() && demangled.back() == '\n')
			{
				demangled.pop_back();
			}

			return demangled.empty() ? p_function_name : demangled;
		}

		auto parse_symbol(const std::string& p_symbol) -> std::string
		{
			auto paren_start = p_symbol.find('(');
			auto paren_end	 = p_symbol.find(')', paren_start);

			if (paren_start != std::string::npos && paren_end != std::string::npos)
			{
				std::string func_info = p_symbol.substr(paren_start + 1, paren_end - paren_start - 1);

				auto plus_pos = func_info.find('+');
				if (plus_pos != std::string::npos)
				{
					func_info = func_info.substr(0, plus_pos);
				}

				if (!func_info.empty())
				{
					return func_info;
				}
			}

			// Fallback: return the original symbol if parsing fails
			return p_symbol;
		}

	public:
		// Setters/Getters
		auto set_dump_on_destroy(bool p_dump) -> void { m_dump_on_destroy = p_dump; }

		auto get_dump_on_destroy() const -> bool { return m_dump_on_destroy; }
	};

	// The actual handler -------------------------------------------------
	static void crash_handler(int sig, siginfo_t*, void*)
	{
		std::cerr << "\n*** Fatal signal " << sig << " received – dumping stack ***\n";
		stack_trace st;
		st.gather_current_stack();
		st.dump_stack();
		std::cout.flush();
		std::exit(128 + sig);
	}

	struct registrar
	{
		registrar()
		{
			const std::vector<int> fatal_signals = {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGINT, SIGTERM, SIGHUP, SIGQUIT, SIGPIPE, SIGALRM, SIGUSR1, SIGUSR2};
			register_signals(fatal_signals);
		}

		void register_signals(const std::vector<int>& signums)
		{
			std::lock_guard<std::mutex> lk(mtx_);	 // once
			if (installed_)
				return;

			struct sigaction sa;
			std::memset(&sa, 0, sizeof(sa));
			sa.sa_sigaction = &crash_handler;
			sa.sa_flags		= static_cast<std::int32_t>(SA_SIGINFO | SA_RESETHAND);	   // one-shot
			for (int s : signums)
				::sigaction(s, &sa, nullptr);

			installed_ = true;
		}

	private:
		std::mutex mtx_;
		bool installed_;
	};
}	 // namespace global_utils
#endif	  // STACK_TRACE_HPP
