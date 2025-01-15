#pragma once
#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sstream>

#include "math_utils.hpp" // IWYU pragma: export

namespace utils
{
	template <typename in_type_t> [[nodiscard]] inline auto is_between(const in_type_t& in_var, const in_type_t& in_lhs, const in_type_t& in_rhs) -> bool
	{
		return in_var >= in_lhs && in_var <= in_rhs;
	}

	
// Function to format numbers with single quotes
	inline auto format_quotes(std::size_t num) -> std::string
	{
		std::ostringstream oss;
		std::string num_str = std::to_string(num);

		int counter = 0;
		for (auto it = num_str.rbegin(); it != num_str.rend(); ++it)
		{
			if (counter == 3)
			{
				oss << '\'';
				counter = 0;
			}
			oss << *it;
			++counter;
		}

		std::string formatted = oss.str();
		std::reverse(formatted.begin(), formatted.end());
		return formatted;
	}
} // namespace utils

#endif // UTILS_HPP
