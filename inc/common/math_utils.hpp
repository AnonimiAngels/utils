#pragma once
#ifndef MATH_UTIL_HPP
#define MATH_UTIL_HPP

#include <cstdlib>
#include <random>

namespace utils
{
	template <typename in_type_t = std::int32_t> class math_utility
	{
	  private:
		mutable std::mt19937 m_rand_gen;
		std::random_device m_seed;

	  public:
		math_utility() { m_rand_gen.seed(m_seed()); }

		// @return a random number between [min, max]
		auto p_caos(in_type_t min, in_type_t max) const -> in_type_t
		{
			// Based on type of in_type_t, we can use different distributions
			if constexpr (std::is_same_v<in_type_t, std::int32_t>)
			{
				std::uniform_int_distribution<std::int32_t> dist(min, max);
				return dist(m_rand_gen);
			}
			else if constexpr (std::is_same_v<in_type_t, std::int64_t>)
			{
				std::uniform_int_distribution<std::int64_t> dist(min, max);
				return dist(m_rand_gen);
			}
			else if constexpr (std::is_same_v<in_type_t, std::uint32_t>)
			{
				std::uniform_int_distribution<std::uint32_t> dist(min, max);
				return dist(m_rand_gen);
			}
			else if constexpr (std::is_same_v<in_type_t, std::uint64_t>)
			{
				std::uniform_int_distribution<std::uint64_t> dist(min, max);
				return dist(m_rand_gen);
			}
			else if constexpr (std::is_same_v<in_type_t, std::float_t>)
			{
				std::uniform_real_distribution<std::float_t> dist(min, max);
				return dist(m_rand_gen);
			}
			else if constexpr (std::is_same_v<in_type_t, std::double_t>)
			{
				std::uniform_real_distribution<std::double_t> dist(min, max);
				return dist(m_rand_gen);
			}
			else { static_assert(std::is_same_v<in_type_t, in_type_t>, "Unsupported type"); }
		}
		// @return a random number between [min, max - 1]
		auto p_caos_ex(in_type_t min, in_type_t max) const -> in_type_t { return p_caos(min, max - 1); }
	};
} // namespace utils

#endif // MATH_UTIL_HPP
