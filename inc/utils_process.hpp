#pragma once
#include <cstdio>
#ifndef PROCESS_HPP
	#define PROCESS_HPP

	#include <array>
	#include <cstdint>
	#include <future>
	#include <string>

	#include <sys/wait.h>
	#include <unistd.h>

	#include "filesystem.hpp"	   // IWYU pragma: keep Provides cxx11 fs::
	#include "format.hpp"		   // IWYU pragma: keep Provides cxx11 std::format std::print
	#include "ranges.hpp"		   // IWYU pragma: keep Provides cxx11 ranges::
	#include "utils_macros.hpp"	   // IWYU pragma: keep Provides cxx11 MACRO_CXX{14,17,20,23,26}_ENABLED

	#if defined(PROCESS_VERBOSE)
		#define PROCESS_DEBUG(...)                                                                                                                   \
			std::print(__VA_ARGS__);                                                                                                                 \
			std::fflush(stdout)
	#else
		#define PROCESS_DEBUG(...) asm("nop")
	#endif

class process
{
public:
	using self_t = process;

private:
	std::string m_buffer;
	std::int32_t m_return_code = 0;
	std::size_t m_buffer_size  = static_cast<std::size_t>(64 * 1024);
	std::future<void> m_future;

public:
	~process() { wait(); }

	process() = default;

	process(const std::string& p_cmd, std::size_t p_buffer_size = static_cast<std::size_t>(64 * 1024), bool p_is_async = false)
		: m_buffer_size(p_buffer_size)
	{
		execute(p_cmd, p_is_async);
	}

	process(const self_t&)					 = delete;
	auto operator=(const self_t&) -> self_t& = delete;

	process(self_t&& p_other)					= default;
	auto operator=(self_t&& p_other) -> self_t& = default;

public:
	auto get_output() -> const std::string& { return m_buffer; }

	MACRO_NODISCARD auto get_return_code() const -> std::int32_t { return m_return_code; }

	auto execute(const std::string& p_cmd, bool p_is_async = false) -> void
	{
		if (p_is_async)
		{
			execute_async(p_cmd);
		}
		else
		{
			execute_sync(p_cmd);
		}
	}

	auto wait() -> void
	{
		if (m_future.valid())
		{
			m_future.wait();
		}
	}

private:
	auto execute_async(const std::string& p_cmd) -> void
	{
		m_future = std::async(std::launch::async, [this, p_cmd] { execute_sync(p_cmd); });
	}

	auto execute_sync(const std::string& p_cmd) -> void
	{
		std::string sanitized_cmd = p_cmd;
		for (auto& letter : sanitized_cmd)
		{
			if (letter == '\n' || letter == '\r')
			{
				letter = ' ';
			}
		}

		std::string::size_type pos = 0;
		while ((pos = sanitized_cmd.find("  ", pos)) != std::string::npos)
		{
			sanitized_cmd.erase(pos, 1);
		}

		auto start = sanitized_cmd.find_first_not_of(' ');
		if (start == std::string::npos)
		{
			sanitized_cmd.clear();
		}
		else
		{
			auto end	  = sanitized_cmd.find_last_not_of(' ');
			sanitized_cmd = sanitized_cmd.substr(start, end - start + 1);
		}

		PROCESS_DEBUG("DEBUG: Executing command: '{}'\n", sanitized_cmd);

		std::array<std::int32_t, 2> pipe_fd = {-1, -1};
		if (pipe(pipe_fd.data()) == -1)
		{
			PROCESS_DEBUG("DEBUG: Failed to create pipe\n");
			m_buffer	  = "Error: Failed to create pipe";
			m_return_code = -1;
			return;
		}

		pid_t pid = fork();
		if (pid == -1)
		{
			PROCESS_DEBUG("DEBUG: Failed to fork process\n");
			close(pipe_fd[0]);
			close(pipe_fd[1]);
			m_buffer	  = "Error: Failed to fork process";
			m_return_code = -1;
			return;
		}

		if (pid == 0)
		{
			close(pipe_fd[0]);

			dup2(pipe_fd[1], STDOUT_FILENO);
			dup2(pipe_fd[1], STDERR_FILENO);
			close(pipe_fd[1]);

			execl("/bin/bash", "bash", "-c", sanitized_cmd.c_str(), nullptr);

			_exit(127);
		}
		else
		{
			close(pipe_fd[1]);

			m_buffer.clear();
			m_buffer.reserve(m_buffer_size);
			std::array<char, 4096> buffer = {};
			ssize_t bytes_read			  = 0;

			while ((bytes_read = read(pipe_fd[0], buffer.data(), buffer.size())) > 0)
			{
				m_buffer.append(buffer.data(), static_cast<std::size_t>(bytes_read));
			}

			close(pipe_fd[0]);

			std::int32_t status = 0;
			if (waitpid(pid, &status, 0) == -1)
			{
				PROCESS_DEBUG("DEBUG: Failed to wait for child process\n");
				m_return_code = -1;
				return;
			}

			if (WIFEXITED(status))
			{
				m_return_code = WEXITSTATUS(status);
			}
			else if (WIFSIGNALED(status))
			{
				m_return_code = -WTERMSIG(status);
			}
			else
			{
				m_return_code = -1;
			}

			if (!m_buffer.empty() && m_buffer.back() == '\n')
			{
				m_buffer.pop_back();
			}

			PROCESS_DEBUG("DEBUG: Process completed - Return code: {}, Output: '{}'\n", m_return_code, m_buffer);
		}
	}
};

#endif