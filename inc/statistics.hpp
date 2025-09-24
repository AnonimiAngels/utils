/**
 * @file statistics.hpp
 * @brief Statistical utilities
 * @contains Generic averager for statistical calculations
 */

#pragma once
#ifndef STATISTICS_HPP
	#define STATISTICS_HPP

	#include <cmath>
	#include <cstdint>
	#include <limits>
	#include <string>
	#include <vector>

	#include "expected.hpp"
	#include "ranges.hpp"

namespace utils
{

	/**
	 * @brief Generic averager for statistical calculations
	 * @tparam value_t Type of values to average
	 */
	template <typename value_t> class averager
	{
	public:
		// Type aliases
		using self_t	 = averager;
		using val_t		 = value_t;
		using val_cref_t = const value_t&;
		using size_t	 = std::size_t;

	private:
		// Member variables
		std::vector<val_t> m_samples;
		std::uint32_t m_max_samples;
		std::uint32_t m_cur_idx;

		mutable val_t m_cached_max;
		mutable val_t m_cached_min;
		mutable val_t m_cached_sum;

		mutable bool m_is_dirty_max = true;
		mutable bool m_is_dirty_min = true;
		mutable bool m_is_dirty_sum = true;

	public:
		/**
		 * @brief Default constructor
		 */
		averager() noexcept : m_samples(), m_max_samples(std::numeric_limits<std::uint32_t>::max()), m_cur_idx(0) {}

		/**
		 * @brief Constructor with sample count
		 * @param p_sample_cnt Maximum number of samples
		 */
		explicit averager(std::uint32_t p_sample_cnt) noexcept : m_samples(), m_max_samples(p_sample_cnt), m_cur_idx(0) { m_samples.reserve(m_max_samples); }

	public:
		/**
		 * @brief Add sample value
		 * @param p_sample Sample value to add
		 */
		auto add_sample(val_cref_t p_sample) noexcept -> void
		{
			if (m_samples.size() < m_max_samples)
			{
				m_samples.push_back(p_sample);
			}
			else
			{
				m_samples[m_cur_idx] = p_sample;
			}

			m_cur_idx = (m_cur_idx + 1) % m_max_samples;

			invalidate_cache();
		}

		/**
		 * @brief Get average of samples
		 * @return Expected containing average or error
		 */
		auto get_avg() const noexcept -> utils::expected<val_t, std::string>
		{
			if (m_samples.empty())
			{
				return utils::make_unexpected(std::string("No samples"));
			}

			auto smp_cnt = static_cast<std::int32_t>(m_samples.size());
			if (smp_cnt == 0)
			{
				return utils::make_unexpected(std::string("No samples"));
			}

			const auto& sum = get_sum();
			return sum / smp_cnt;
		}

		/**
		 * @brief Get minimum sample
		 * @return Minimum value
		 */
		auto get_min() const noexcept -> val_t
		{
			if (m_samples.empty())
			{
				return val_t(0);
			}

			if (!m_is_dirty_min)
			{
				return m_cached_min;
			}

			m_is_dirty_min = false;
			auto iter	   = ranges::min_element(m_samples);
			m_cached_min   = (iter != m_samples.end()) ? *iter : val_t(0);
			return m_cached_min;
		}

		/**
		 * @brief Get maximum sample
		 * @return Maximum value
		 */
		auto get_max() const noexcept -> val_t
		{
			if (m_samples.empty())
			{
				return val_t(0);
			}

			if (!m_is_dirty_max)
			{
				return m_cached_max;
			}

			m_is_dirty_max = false;
			auto iter	   = ranges::max_element(m_samples);
			m_cached_max   = (iter != m_samples.end()) ? *iter : val_t(0);
			return m_cached_max;
		}

		/**
		 * @brief Get sum of all samples
		 * @return Sum of samples
		 */
		auto get_sum() const noexcept -> val_t
		{
			if (m_samples.empty())
			{
				return val_t(0);
			}

			if (!m_is_dirty_sum)
			{
				return m_cached_sum;
			}

			m_is_dirty_sum = false;
			auto sum	   = ranges::accumulate(m_samples, val_t(0));
			m_cached_sum   = sum;
			return m_cached_sum;
		}

		/**
		 * @brief Get sample count
		 * @return Current number of samples
		 */
		auto get_smp_cnt() const noexcept -> std::size_t { return m_samples.size(); }

		/**
		 * @brief Get all samples
		 * @return Vector of samples
		 */
		auto get_samples() const noexcept -> const std::vector<val_t>& { return m_samples; }

		/**
		 * @brief Clear all samples
		 */
		auto clear_smps() noexcept -> void
		{
			m_samples.clear();
			m_cur_idx = 0;
		}

		/**
		 * @brief Reset averager
		 */
		auto reset() noexcept -> void { clear_smps(); }

		/**
		 * @brief Get standard deviation of samples
		 * @return Expected containing std deviation or error
		 */
		auto get_std_dev() const noexcept -> utils::expected<val_t, std::string>
		{
			if (m_samples.size() < 2)
			{
				return val_t(0);
			}

			auto avg_res = get_avg();
			if (!avg_res.has_value())
			{
				return utils::make_unexpected(avg_res.error());
			}

			auto avg			 = avg_res.value();
			auto lambda_variance = [avg](val_t p_acc, val_cref_t p_val) -> val_t
			{
				auto diff = p_val - avg;
				return p_acc + (diff * diff);
			};

			auto smp_cnt = static_cast<std::int32_t>(m_samples.size());
			if (smp_cnt == 0)
			{
				return utils::make_unexpected(std::string("No samples"));
			}

			auto var_sum  = ranges::accumulate(m_samples, val_t(0), lambda_variance);
			auto variance = var_sum / smp_cnt;

			// Handle different types for sqrt
			using std::sqrt;
			return val_t(sqrt(static_cast<double>(variance)));
		}

		/**
		 * @brief Get variance of samples
		 * @return Expected containing variance or error
		 */
		auto get_variance() const noexcept -> utils::expected<val_t, std::string>
		{
			if (m_samples.size() < 2)
			{
				return val_t(0);
			}

			auto avg_res = get_avg();
			if (!avg_res.has_value())
			{
				return utils::make_unexpected(avg_res.error());
			}

			auto avg			 = avg_res.value();
			auto lambda_variance = [avg](val_t p_acc, val_cref_t p_val) -> val_t
			{
				auto diff = p_val - avg;
				return p_acc + (diff * diff);
			};

			auto smp_cnt = static_cast<std::int32_t>(m_samples.size());
			if (smp_cnt == 0)
			{
				return utils::make_unexpected(std::string("No samples"));
			}

			auto var_sum = ranges::accumulate(m_samples, val_t(0), lambda_variance);
			return var_sum / smp_cnt;
		}

		/**
		 * @brief Check if averager is empty
		 * @return True if no samples
		 */
		auto empty() const noexcept -> bool { return m_samples.empty(); }

		/**
		 * @brief Set maximum sample count
		 * @param p_max New maximum sample count
		 */
		auto set_max_samples(std::uint32_t p_max) noexcept -> void
		{
			m_max_samples = p_max;
			if (m_samples.size() > m_max_samples)
			{
				m_samples.resize(m_max_samples);
				m_cur_idx = 0;
			}
			else if (m_cur_idx >= m_max_samples)
			{
				m_cur_idx = 0;
			}
		}

		/**
		 * @brief Get maximum sample count
		 * @return Maximum number of samples
		 */
		auto get_max_samples() const noexcept -> std::uint32_t { return m_max_samples; }

		/**
		 * @brief Check if at capacity
		 * @return True if sample count equals max samples
		 */
		auto is_full() const noexcept -> bool { return m_samples.size() >= m_max_samples; }

	private:
		auto invalidate_cache() noexcept -> void
		{
			m_is_dirty_max = true;
			m_is_dirty_min = true;
			m_is_dirty_sum = true;
		}
	};

	/**
	 * @brief Calculate proximity percentage of p_min relative to p_max.
	 *
	 * - Returns value in the closed range [0, 100].
	 * - If p_min > p_max the values are swapped (order-insensitive).
	 * - If both are zero -> returns 100 (identical zero values).
	 * - If p_max == 0 and p_min != 0 -> returns 0 (cannot be close to zero).
	 *
	 * @tparam value_t arithmetic type (integral or floating)
	 * @param p_p_min left value
	 * @param p_p_max right (reference) value
	 * @return value_t percentage in [0,100]
	 */
	template <typename value_t> auto calc_proximity_pct(value_t p_p_min, value_t p_p_max) -> value_t
	{
		static_assert(std::is_arithmetic<value_t>::value, "value_t must be arithmetic type");

		/* ensure non-decreasing order */
		if (p_p_min > p_p_max)
		{
			std::swap(p_p_min, p_p_max);
		}

		/* handle division-by-zero / special zero-case */
		if (p_p_max == static_cast<value_t>(0))
		{
			return (p_p_min == static_cast<value_t>(0)) ? static_cast<value_t>(100) : static_cast<value_t>(0);
		}

		/* compute linear ratio in double for precision */
		double ratio = (static_cast<double>(p_p_min) / static_cast<double>(p_p_max)) * 100.0;

		/* clamp to [0,100] */
		ratio = std::max(0.0, std::min(100.0, ratio));

		/* return rounded integer for integral types, exact double-cast for floating types */
		if (std::is_integral<value_t>::value)
		{
			return static_cast<value_t>(std::round(ratio));
		}

		return static_cast<value_t>(ratio);
	}

}	 // namespace utils

#endif	  // STATISTICS_HPP
