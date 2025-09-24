#pragma once
#ifndef UTILITY_TYPES_HPP
	#define UTILITY_TYPES_HPP

namespace utils
{
	/**
	 * @brief Tag type for in-place construction
	 */
	struct in_place_t
	{
		explicit in_place_t() = default;
	};

	constexpr in_place_t in_place{};

}	 // namespace utils

#endif	  // UTILITY_TYPES_HPP