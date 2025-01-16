#pragma once
#include <algorithm>
#ifndef TIMER_HPP
#define TIMER_HPP

/**
 * @file timer.hpp
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

#include <chrono>
#include <thread>
#include <cstdint>

namespace utils
{
	template <class T> class precision_timer
	{
	  public:
		using m_type	   = T;
		using m_crtype	   = const m_type&;
		using m_type_tick  = std::uint64_t;
		using m_type_point = std::chrono::steady_clock::time_point;

		precision_timer() : precision_timer(m_type(0), true) {}
		precision_timer(bool auto_start) : precision_timer(m_type(0), auto_start) {}
		precision_timer(m_crtype elapse_time, bool auto_start = true) { self_init(auto_start, elapse_time, get_now()); }

		auto start() -> void
		{
			m_start_time = get_now();
			m_started	 = true;
		}

		auto stop() -> void
		{
			m_started = false;
			m_elapsed = std::chrono::duration_cast<m_type>(get_now() - m_start_time);
		}

		[[nodiscard]] auto is_started() const -> bool { return m_started; }

		auto is_elapsed() -> bool { return get_elapsed() >= m_elapse; }

		auto restart() -> void { m_start_time = get_now(); }

		auto set_elapse(m_crtype elapse_time) -> void { m_elapse = elapse_time; }

		auto set_elapse(m_type_tick elapse_time) -> void { m_elapse = m_type(elapse_time); }

		[[nodiscard]] auto get_elapse() const -> m_crtype { return m_elapse; }

		auto get_elapsed() -> m_crtype
		{
			if (m_started) { m_elapsed = std::chrono::duration_cast<m_type>(get_now() - m_start_time); }
			return m_elapsed;
		}

		auto get_elapsed_and_restart() -> m_crtype
		{
			get_elapsed();
			restart();
			return m_elapsed;
		}

		auto get_ticks() -> m_type_tick { return get_elapsed().count(); }
		template <typename in_ret_t> [[nodiscard]] constexpr auto get_as() -> in_ret_t { return std::chrono::duration_cast<in_ret_t>(get_elapsed()); }

		template <typename U> auto wait_elapse(U in_sleep_time) -> void
		{
			while (!is_elapsed()) { std::this_thread::sleep_for(in_sleep_time); }
		}

	  private:
		static auto get_now() -> m_type_point { return std::chrono::steady_clock::now(); }

		auto self_init(bool in_auto_start, m_type in_elapse, m_type_point in_start) -> void
		{
			m_started	 = in_auto_start;
			m_elapse	 = in_elapse;
			m_elapsed	 = m_type(0);
			m_start_time = in_start;
		}

		m_type m_elapse{};
		m_type m_elapsed{};
		m_type_point m_start_time;
		bool m_started{};
	};
} // namespace utils
#endif // TIMER_HPP
