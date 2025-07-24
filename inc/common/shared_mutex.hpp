#pragma once
#ifndef SHARED_MUTEX_HPP
#define SHARED_MUTEX_HPP

/**
 * @file shared_mutex.hpp
 * @author anonimi_angels (gutsulyak.vladyslav2000@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-7-24
 *
 * @copyright
 *The MIT License (MIT)
 *Copyright (c) 2025 anonimi_angels
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

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <system_error>

namespace utils
{
	class shared_mutex
	{
	public:
		using self_t			 = shared_mutex;
		using native_handle_type = std::mutex::native_handle_type;

		struct constants
		{
			static constexpr std::uint32_t k_max_writers_waiting = 0xFFFFFFFFU;
			static constexpr std::uint64_t k_max_readers		 = std::numeric_limits<std::uint64_t>::max();
		};

	private:
		std::mutex m_mutex;

		std::condition_variable m_gate_1;
		std::condition_variable m_gate_2;

		std::uint32_t m_writers_waiting = 0U;
		std::uint32_t m_readers_waiting = 0U;
		std::uint32_t m_writers			= 0U;
		std::uint64_t m_readers			= 0U;

		static constexpr std::uint64_t max_readers = constants::k_max_readers;

	public:
		~shared_mutex()
		{
#ifndef NDEBUG
			if (m_writers != 0U || m_readers != 0U || m_writers_waiting != 0U || m_readers_waiting != 0U)
			{
				std::terminate();
			}
#endif
		}

		shared_mutex() noexcept						 = default;
		shared_mutex(const self_t&)					 = delete;
		auto operator=(const self_t&) -> self_t&	 = delete;
		shared_mutex(self_t&&) noexcept				 = delete;
		auto operator=(self_t&&) noexcept -> self_t& = delete;

		auto lock() -> void
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_writers_waiting == constants::k_max_writers_waiting)
			{
				throw std::system_error(std::make_error_code(std::errc::value_too_large));
			}
			++m_writers_waiting;

			m_gate_1.wait(lock, [this] { return (m_readers == 0U) && (m_writers == 0U); });
			--m_writers_waiting;
			m_writers = 1U;
		}

		auto try_lock() noexcept -> bool
		{
			const std::lock_guard<std::mutex> lock(m_mutex);
			if ((m_readers == 0U) && (m_writers == 0U))
			{
				m_writers = 1U;
				return true;
			}
			return false;
		}

		template <typename rep, typename period> auto try_lock_for(const std::chrono::duration<rep, period>& p_rel_time) -> bool
		{
			return try_lock_until(std::chrono::steady_clock::now() + p_rel_time);
		}

		template <typename clock, typename duration> auto try_lock_until(const std::chrono::time_point<clock, duration>& p_abs_time) -> bool
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_writers_waiting == constants::k_max_writers_waiting)
			{
				throw std::system_error(std::make_error_code(std::errc::value_too_large));
			}
			++m_writers_waiting;

			bool success = m_gate_1.wait_until(lock, p_abs_time, [this] { return (m_readers == 0U) && (m_writers == 0U); });
			--m_writers_waiting;
			if (success)
			{
				m_writers = 1U;
			}
			return success;
		}

		auto unlock() -> void
		{
			const std::lock_guard<std::mutex> lock(m_mutex);
			if (m_writers == 0U)
			{
				throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
			}

			m_writers = 0U;
			if (m_readers_waiting != 0U)
			{
				m_gate_2.notify_all();
			}
			else if (m_writers_waiting != 0U)
			{
				m_gate_1.notify_one();
			}
		}

		auto lock_shared() -> void
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_readers == max_readers)
			{
				throw std::system_error(std::make_error_code(std::errc::value_too_large));
			}
			++m_readers_waiting;

			m_gate_2.wait(lock, [this] { return (m_writers == 0U) && (m_writers_waiting == 0U); });
			--m_readers_waiting;
			++m_readers;
		}

		auto try_lock_shared() noexcept -> bool
		{
			const std::lock_guard<std::mutex> lock(m_mutex);
			if ((m_writers == 0U) && (m_writers_waiting == 0U) && (m_readers != max_readers))
			{
				++m_readers;
				return true;
			}
			return false;
		}

		template <typename rep, typename period> auto try_lock_shared_for(const std::chrono::duration<rep, period>& p_rel_time) -> bool
		{
			return try_lock_shared_until(std::chrono::steady_clock::now() + p_rel_time);
		}

		template <typename clock, typename duration> auto try_lock_shared_until(const std::chrono::time_point<clock, duration>& p_abs_time) -> bool
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_readers == max_readers)
			{
				throw std::system_error(std::make_error_code(std::errc::value_too_large));
			}
			++m_readers_waiting;

			bool success = m_gate_2.wait_until(lock, p_abs_time, [this] { return (m_writers == 0U) && (m_writers_waiting == 0U); });
			--m_readers_waiting;
			if (success)
			{
				++m_readers;
			}
			return success;
		}

		auto unlock_shared() -> void
		{
			const std::lock_guard<std::mutex> lock(m_mutex);
			if (m_readers == 0U)
			{
				throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
			}

			--m_readers;
			if ((m_readers == 0U) && (m_writers_waiting != 0U))
			{
				m_gate_1.notify_one();
			}
		}

		auto native_handle() -> native_handle_type { return m_mutex.native_handle(); }
	};
}	 // namespace utils

#endif	  // SHARED_MUTEX_HPP
