#pragma once
#ifndef PROCCESS_HPP
#define PROCCESS_HPP

/**
 * @file proccess.hpp
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

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <future>

class process
{
  private:
	FILE* m_pipe = nullptr;

	std::string m_buffer;

	int32_t m_return_code = 0;
	size_t m_buffer_size  = static_cast<size_t>(64 * 1024);

	std::future<void> m_future;

  public:
	process() = default;
	process(const std::string_view in_cmd, size_t in_read_buffer_size = static_cast<size_t>(64 * 1024), bool is_async = false) : m_buffer_size(in_read_buffer_size)
	{
		execute(in_cmd, is_async);
	}

	~process() { wait(); }

	[[nodiscard]] auto get_output() -> const std::string& { return m_buffer; }
	[[nodiscard]] auto get_return_code() const -> int32_t { return m_return_code; }

	auto execute(const std::string_view in_cmd, bool is_async = false) -> void { is_async ? execute_cmd_async(in_cmd) : execute_cmd(in_cmd); }

	auto wait() -> void
	{
		if (m_future.valid()) { m_future.wait(); }
	}

  private:
	auto execute_cmd_async(const std::string_view in_cmd) -> void
	{
		m_future = std::async(std::launch::async, [this, in_cmd] { execute_cmd(in_cmd); });
	}

	auto execute_cmd(const std::string_view in_cmd) -> void
	{
		m_pipe = popen(in_cmd.data(), "r");

		// Read the output of the command into string buffer
		if (m_pipe == nullptr)
		{
			m_buffer	  = "Error: Unable to execute command";
			m_return_code = INT32_MIN;
			return;
		}

		std::vector<char> pipe_buffer(m_buffer_size);
		while ((feof(m_pipe) == 0) && (ferror(m_pipe) == 0))
		{
			if (fgets(pipe_buffer.data(), static_cast<int32_t>(pipe_buffer.size()), m_pipe) != nullptr) { m_buffer += pipe_buffer.data(); }
		}

		if (!m_buffer.empty() && m_buffer.back() == '\n') { m_buffer.pop_back(); }

		m_return_code = pclose(m_pipe);
	}
};

#endif // PROCCESS_HPP
