/**
 * @file precision_timer.hpp
 * @brief Precision timing utilities with multiple timer types
 */

#pragma once
#ifndef PRECISION_TIMER_HPP
	#define PRECISION_TIMER_HPP

	#include <chrono>
	#include <cstdint>
	#include <string>
	#include <thread>
	#include <vector>

	#include "expected.hpp"
	#include "statistics.hpp"

namespace utils
{

	/**
	 * @brief High precision timer base class
	 * @tparam duration_t Chrono duration type for time measurement
	 */
	template <typename duration_t> class precision_timer
	{
	public:
		using self_t	   = precision_timer;
		using dur_t		   = duration_t;
		using dur_cref_t   = const duration_t&;
		using tick_t	   = std::uint64_t;
		using time_point_t = std::chrono::steady_clock::time_point;

	private:
		dur_t m_elapse;
		dur_t m_elapsed;
		time_point_t m_start_time;
		bool m_started;

	public:
		/**
		 * @brief Default constructor
		 */
		precision_timer() noexcept : precision_timer(dur_t(0), true) {}

		/**
		 * @brief Constructor with auto start option
		 * @param p_auto_start Whether to start timer automatically
		 */
		explicit precision_timer(bool p_auto_start) noexcept : precision_timer(dur_t(0), p_auto_start) {}

		/**
		 * @brief Constructor with elapse time
		 * @param p_elapse_time Elapse time duration
		 * @param p_auto_start Whether to start timer automatically
		 */
		precision_timer(dur_cref_t p_elapse_time, bool p_auto_start = true) noexcept : m_elapse(p_elapse_time), m_elapsed(dur_t(0)), m_start_time(get_now()), m_started(p_auto_start) {}

	protected:
		/**
		 * @brief Get current time point
		 * @return Current steady clock time point
		 */
		static auto get_now() noexcept -> time_point_t { return std::chrono::steady_clock::now(); }

	public:
		/**
		 * @brief Start timing
		 */
		auto start() noexcept -> void
		{
			m_start_time = get_now();
			m_started	 = true;
		}

		/**
		 * @brief Stop timing
		 */
		auto stop() noexcept -> void
		{
			m_started = false;
			m_elapsed = std::chrono::duration_cast<dur_t>(get_now() - m_start_time);
		}

		/**
		 * @brief Check if timer is started
		 * @return True if timer is running
		 */
		auto is_started() const noexcept -> bool { return m_started; }

		/**
		 * @brief Check if elapsed time exceeded elapse threshold
		 * @return True if elapsed >= elapse
		 */
		auto is_elapsed() noexcept -> bool { return get_elapsed() >= m_elapse; }

		/**
		 * @brief Restart timer
		 */
		auto restart() noexcept -> void { m_start_time = get_now(); }

		/**
		 * @brief Set elapse threshold
		 * @param p_elapse_time New elapse duration
		 */
		auto set_elapse(dur_cref_t p_elapse_time) noexcept -> void { m_elapse = p_elapse_time; }

		/**
		 * @brief Set elapse threshold from ticks
		 * @param p_elapse_time Elapse time in ticks
		 */
		auto set_elapse(tick_t p_elapse_time) noexcept -> void { m_elapse = dur_t(p_elapse_time); }

		/**
		 * @brief Get elapse threshold
		 * @return Elapse duration reference
		 */
		auto get_elapse() const noexcept -> dur_cref_t { return m_elapse; }

		/**
		 * @brief Get elapsed time
		 * @return Elapsed duration reference
		 */
		auto get_elapsed() noexcept -> dur_cref_t
		{
			if (m_started)
			{
				m_elapsed = std::chrono::duration_cast<dur_t>(get_now() - m_start_time);
			}
			return m_elapsed;
		}

		/**
		 * @brief Get elapsed time as different duration type
		 * @tparam cast_dur_t Target duration type
		 * @return Elapsed time in target duration
		 */
		template <typename cast_dur_t> auto get_cast_elapsed() const noexcept -> cast_dur_t
		{
			if (m_started)
			{
				return std::chrono::duration_cast<cast_dur_t>(get_now() - m_start_time);
			}
			return std::chrono::duration_cast<cast_dur_t>(m_elapsed);
		}

		/**
		 * @brief Get time in specific units
		 * @tparam cast_dur_t Duration type for conversion
		 * @return Time count in specified units
		 */
		template <typename cast_dur_t> auto get_time() noexcept -> tick_t { return std::chrono::duration_cast<cast_dur_t>(get_elapsed()).count(); }

		/**
		 * @brief Get elapsed and restart
		 * @return Elapsed duration reference
		 */
		auto get_elapsed_restart() noexcept -> dur_cref_t
		{
			get_elapsed();
			restart();
			return m_elapsed;
		}

		/**
		 * @brief Get elapsed ticks
		 * @return Tick count
		 */
		auto get_ticks() noexcept -> tick_t { return get_elapsed().count(); }

		/**
		 * @brief Wait until elapse time reached
		 * @tparam sleep_dur_t Sleep duration type
		 * @param p_sleep_time Sleep duration between checks
		 */
		template <typename sleep_dur_t> auto wait_elapse(sleep_dur_t p_sleep_time) noexcept -> void
		{
			while (!is_elapsed())
			{
				std::this_thread::sleep_for(p_sleep_time);
			}
		}
	};

	/**
	 * @brief Timer with sample averaging capabilities
	 * @tparam duration_t Chrono duration type
	 */
	template <typename duration_t, typename average_t> class average_timer : public precision_timer<duration_t>
	{
	public:
		using self_t  = average_timer;
		using base_t  = precision_timer<duration_t>;
		using value_t = average_t;
		using typename base_t::dur_cref_t;
		using typename base_t::dur_t;
		using typename base_t::tick_t;

	private:
		averager<value_t> m_averager;

	public:
		/**
		 * @brief Default constructor
		 */
		explicit average_timer() noexcept : base_t(false), m_averager() {}

		/**
		 * @brief Constructor with sample count
		 * @param p_sample_cnt Maximum number of samples
		 */
		explicit average_timer(std::uint32_t p_sample_cnt) noexcept : base_t(false), m_averager(p_sample_cnt) {}

		average_timer(const self_t&)			 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		average_timer(self_t&&) noexcept			 = default;
		auto operator=(self_t&&) noexcept -> self_t& = default;

		/**
		 * @brief Add current elapsed as sample
		 */
		auto add_sample() noexcept -> void
		{
			auto elapsed = this->get_elapsed();
			add_sample(elapsed);
			this->restart();
		}

		/**
		 * @brief Add specific sample value
		 * @param p_sample Sample duration to add
		 */
		template <typename type_t = dur_t> auto add_sample(type_t p_sample) noexcept -> void
		{
			auto sample_val = static_cast<value_t>(p_sample.count());
			m_averager.add_sample(sample_val);
		}

		/**
		 * @brief Get average of samples
		 * @return Expected containing average or error
		 */
		auto get_avg() const noexcept -> utils::expected<value_t, std::string> { return m_averager.get_avg(); }

		/**
		 * @brief Get average as ticks
		 * @return Expected containing average ticks or error
		 */
		auto get_avg_ticks() const noexcept -> utils::expected<tick_t, std::string>
		{
			auto avg_res = get_avg();
			if (!avg_res.has_value())
			{
				return utils::make_unexpected(avg_res.error());
			}
			return avg_res.value().count();
		}

		/**
		 * @brief Get minimum sample
		 * @return Minimum duration
		 */
		auto get_min() const noexcept -> value_t { return m_averager.get_min(); }

		/**
		 * @brief Get maximum sample
		 * @return Maximum duration
		 */
		auto get_max() const noexcept -> value_t { return m_averager.get_max(); }

		/**
		 * @brief Get sample count
		 * @return Current number of samples
		 */
		auto get_smp_cnt() const noexcept -> std::uint32_t { return m_averager.get_smp_cnt(); }

		/**
		 * @brief Clear all samples
		 */
		auto clear_smps() noexcept -> void { m_averager.clear_smps(); }

		/**
		 * @brief Reset timer and samples
		 */
		auto reset() noexcept -> void
		{
			clear_smps();
			this->restart();
		}

		/**
		 * @brief Get standard deviation of samples
		 * @return Expected containing std deviation or error
		 */
		auto get_std_dev() const noexcept -> utils::expected<value_t, std::string> { return m_averager.get_std_dev(); }
	};

	/**
	 * @brief Countdown timer
	 * @tparam duration_t Chrono duration type
	 */
	template <typename duration_t> class countdown_timer
	{
	public:
		using self_t	   = countdown_timer;
		using dur_t		   = duration_t;
		using dur_cref_t   = const duration_t&;
		using tick_t	   = std::uint64_t;
		using time_point_t = std::chrono::steady_clock::time_point;

	private:
		dur_t m_total_dur;
		time_point_t m_start_time;
		bool m_started;

	public:
		/**
		 * @brief Constructor with countdown duration
		 * @param p_dur Total countdown duration
		 */
		explicit countdown_timer(dur_cref_t p_dur) noexcept : m_total_dur(p_dur), m_start_time(), m_started(false) {}

		/**
		 * @brief Start countdown
		 */
		auto start() noexcept -> void
		{
			m_start_time = std::chrono::steady_clock::now();
			m_started	 = true;
		}

		/**
		 * @brief Stop countdown
		 */
		auto stop() noexcept -> void { m_started = false; }

		/**
		 * @brief Reset countdown
		 * @param p_new_dur New countdown duration (optional)
		 */
		auto reset(dur_cref_t p_new_dur = dur_t(0)) noexcept -> void
		{
			if (p_new_dur != dur_t(0))
			{
				m_total_dur = p_new_dur;
			}
			m_started = false;
		}

		/**
		 * @brief Get remaining time
		 * @return Remaining duration
		 */
		auto get_remaining() const noexcept -> dur_t
		{
			if (!m_started)
			{
				return m_total_dur;
			}

			auto elapsed = std::chrono::duration_cast<dur_t>(std::chrono::steady_clock::now() - m_start_time);

			if (elapsed >= m_total_dur)
			{
				return dur_t(0);
			}

			return m_total_dur - elapsed;
		}

		/**
		 * @brief Check if countdown expired
		 * @return True if countdown reached zero
		 */
		auto is_expired() const noexcept -> bool { return m_started && get_remaining() == dur_t(0); }

		/**
		 * @brief Get progress percentage
		 * @return Expected containing percentage (0-100) or error
		 */
		auto get_progress_pct() const noexcept -> utils::expected<std::uint32_t, std::string>
		{
			if (!m_started)
			{
				return utils::make_unexpected(std::string("Not started"));
			}

			auto total_ticks = m_total_dur.count();
			if (total_ticks == 0)
			{
				return utils::make_unexpected(std::string("Zero duration"));
			}

			auto elapsed = std::chrono::duration_cast<dur_t>(std::chrono::steady_clock::now() - m_start_time);

			auto pct = (elapsed.count() * 100) / total_ticks;
			if (pct > 100)
			{
				pct = 100;
			}

			return static_cast<std::uint32_t>(pct);
		}
	};

	/**
	 * @brief Interval timer for periodic events
	 * @tparam duration_t Chrono duration type
	 */
	template <typename duration_t> class interval_timer
	{
	public:
		using self_t	   = interval_timer;
		using dur_t		   = duration_t;
		using dur_cref_t   = const duration_t&;
		using tick_t	   = std::uint64_t;
		using time_point_t = std::chrono::steady_clock::time_point;

	private:
		dur_t m_interval;
		time_point_t m_last_tick;
		std::uint32_t m_tick_cnt;
		bool m_started;

	public:
		/**
		 * @brief Constructor with interval duration
		 * @param p_interval Interval between ticks
		 */
		explicit interval_timer(dur_cref_t p_interval) noexcept : m_interval(p_interval), m_last_tick(), m_tick_cnt(0), m_started(false) {}

		/**
		 * @brief Start interval timer
		 */
		auto start() noexcept -> void
		{
			m_last_tick = std::chrono::steady_clock::now();
			m_tick_cnt	= 0;
			m_started	= true;
		}

		/**
		 * @brief Stop interval timer
		 */
		auto stop() noexcept -> void { m_started = false; }

		/**
		 * @brief Check if interval elapsed
		 * @return True if interval time passed since last tick
		 */
		auto is_ready() const noexcept -> bool
		{
			if (!m_started)
			{
				return false;
			}

			auto now	 = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<dur_t>(now - m_last_tick);
			return elapsed >= m_interval;
		}

		/**
		 * @brief Process tick if ready
		 * @return True if tick occurred
		 */
		auto tick() noexcept -> bool
		{
			if (!is_ready())
			{
				return false;
			}

			m_last_tick = std::chrono::steady_clock::now();
			++m_tick_cnt;
			return true;
		}

		/**
		 * @brief Get tick count
		 * @return Number of ticks since start
		 */
		auto get_tick_cnt() const noexcept -> std::uint32_t { return m_tick_cnt; }

		/**
		 * @brief Get time until next tick
		 * @return Duration until next interval
		 */
		auto get_time_to_tick() const noexcept -> dur_t
		{
			if (!m_started)
			{
				return dur_t(0);
			}

			auto now	 = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<dur_t>(now - m_last_tick);

			if (elapsed >= m_interval)
			{
				return dur_t(0);
			}

			return m_interval - elapsed;
		}

		/**
		 * @brief Set new interval
		 * @param p_interval New interval duration
		 */
		auto set_interval(dur_cref_t p_interval) noexcept -> void { m_interval = p_interval; }
	};

	/**
	 * @brief Stopwatch timer with lap support
	 * @tparam duration_t Chrono duration type
	 */
	template <typename duration_t> class stopwatch_timer
	{
	public:
		using self_t	   = stopwatch_timer;
		using dur_t		   = duration_t;
		using dur_cref_t   = const duration_t&;
		using time_point_t = std::chrono::steady_clock::time_point;

	private:
		std::vector<dur_t> m_laps;
		time_point_t m_start_time;
		time_point_t m_lap_start;
		dur_t m_total_elapsed;
		bool m_started;
		bool m_paused;

	public:
		/**
		 * @brief Default constructor
		 */
		stopwatch_timer() noexcept : m_laps(), m_start_time(), m_lap_start(), m_total_elapsed(0), m_started(false), m_paused(false) {}

		/**
		 * @brief Start or resume stopwatch
		 */
		auto start() noexcept -> void
		{
			auto now = std::chrono::steady_clock::now();

			if (!m_started)
			{
				m_start_time	= now;
				m_lap_start		= now;
				m_total_elapsed = dur_t(0);
				m_started		= true;
				m_paused		= false;
			}
			else if (m_paused)
			{
				m_start_time = now;
				m_paused	 = false;
			}
		}

		/**
		 * @brief Pause stopwatch
		 */
		auto pause() noexcept -> void
		{
			if (m_started && !m_paused)
			{
				auto now = std::chrono::steady_clock::now();
				m_total_elapsed += std::chrono::duration_cast<dur_t>(now - m_start_time);
				m_paused = true;
			}
		}

		/**
		 * @brief Stop and reset stopwatch
		 */
		auto stop() noexcept -> void
		{
			m_started = false;
			m_paused  = false;
			m_laps.clear();
		}

		/**
		 * @brief Record lap time
		 * @return Lap duration
		 */
		auto lap() noexcept -> dur_t
		{
			if (!m_started || m_paused)
			{
				return dur_t(0);
			}

			auto now	 = std::chrono::steady_clock::now();
			auto lap_dur = std::chrono::duration_cast<dur_t>(now - m_lap_start);
			m_laps.push_back(lap_dur);
			m_lap_start = now;
			return lap_dur;
		}

		/**
		 * @brief Get total elapsed time
		 * @return Total duration
		 */
		auto get_total() noexcept -> dur_t
		{
			if (!m_started)
			{
				return dur_t(0);
			}

			if (m_paused)
			{
				return m_total_elapsed;
			}

			auto now = std::chrono::steady_clock::now();
			return m_total_elapsed + std::chrono::duration_cast<dur_t>(now - m_start_time);
		}

		/**
		 * @brief Get all lap times
		 * @return Vector of lap durations
		 */
		auto get_laps() const noexcept -> const std::vector<dur_t>& { return m_laps; }

		/**
		 * @brief Get lap count
		 * @return Number of recorded laps
		 */
		auto get_lap_cnt() const noexcept -> std::uint32_t { return static_cast<std::uint32_t>(m_laps.size()); }

		/**
		 * @brief Get fastest lap
		 * @return Expected containing fastest lap or error
		 */
		auto get_fastest_lap() const noexcept -> utils::expected<dur_t, std::string>
		{
			if (m_laps.empty())
			{
				return utils::make_unexpected(std::string("No laps"));
			}

			auto min_itr = ranges::min_element(m_laps);
			return *min_itr;
		}

		/**
		 * @brief Get slowest lap
		 * @return Expected containing slowest lap or error
		 */
		auto get_slowest_lap() const noexcept -> utils::expected<dur_t, std::string>
		{
			if (m_laps.empty())
			{
				return utils::make_unexpected(std::string("No laps"));
			}

			auto max_itr = ranges::max_element(m_laps);
			return *max_itr;
		}
	};
}	 // namespace utils

#endif