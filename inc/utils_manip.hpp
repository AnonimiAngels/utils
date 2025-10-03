#pragma once
#ifndef UTILS_MANIP
	#define UTILS_MANIP

	#include "format.hpp"
	#include "precision_timer.hpp"

template <typename duration_t> inline auto time_to_string(utils::precision_timer<duration_t>& p_timer) -> std::string
{
	auto elapsed_time = p_timer.get_elapsed();

	constexpr std::uint64_t ns_per_us	= 1000;
	constexpr std::uint64_t us_per_ms	= 1000;
	constexpr std::uint64_t ms_per_sec	= 1000;
	constexpr std::uint64_t sec_per_min = 60;
	constexpr std::uint64_t min_per_hr	= 60;
	constexpr std::uint64_t hr_per_day	= 24;
	constexpr std::uint64_t day_per_yr	= 365;

	constexpr std::uint64_t ns_per_ms  = ns_per_us * us_per_ms;
	constexpr std::uint64_t ns_per_sec = ns_per_ms * ms_per_sec;
	constexpr std::uint64_t ns_per_min = ns_per_sec * sec_per_min;
	constexpr std::uint64_t ns_per_hr  = ns_per_min * min_per_hr;
	constexpr std::uint64_t ns_per_day = ns_per_hr * hr_per_day;
	constexpr std::uint64_t ns_per_yr  = ns_per_day * day_per_yr;

	auto total_nanosec = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed_time).count();

	const std::uint32_t years = static_cast<std::uint32_t>(total_nanosec / ns_per_yr);
	total_nanosec %= ns_per_yr;

	const std::uint32_t days = static_cast<std::uint32_t>(total_nanosec / ns_per_day);
	total_nanosec %= ns_per_day;

	const std::uint32_t hours = static_cast<std::uint32_t>(total_nanosec / ns_per_hr);
	total_nanosec %= ns_per_hr;

	const std::uint32_t minutes = static_cast<std::uint32_t>(total_nanosec / ns_per_min);
	total_nanosec %= ns_per_min;

	const std::uint32_t seconds = static_cast<std::uint32_t>(total_nanosec / ns_per_sec);
	total_nanosec %= ns_per_sec;

	const std::uint32_t milliseconds = static_cast<std::uint32_t>(total_nanosec / ns_per_ms);
	total_nanosec %= ns_per_ms;

	const std::uint32_t microseconds = static_cast<std::uint32_t>(total_nanosec / ns_per_us);
	total_nanosec %= ns_per_us;

	const std::uint32_t nanoseconds = static_cast<std::uint32_t>(total_nanosec);

	std::vector<std::string> parts;
	parts.reserve(8);

	if (years > 0)
	{
		parts.emplace_back(std::format("{}y", years));
	}
	if (days > 0)
	{
		parts.emplace_back(std::format("{}d", days));
	}
	if (hours > 0)
	{
		parts.emplace_back(std::format("{}h", hours));
	}
	if (minutes > 0)
	{
		parts.emplace_back(std::format("{}min", minutes));
	}
	if (seconds > 0)
	{
		parts.emplace_back(std::format("{:03d}s", seconds));
	}
	if (milliseconds > 0)
	{
		parts.emplace_back(std::format("{:03d}ms", milliseconds));
	}
	if (microseconds > 0)
	{
		parts.emplace_back(std::format("{:03d}μs", microseconds));
	}
	if (nanoseconds > 0 || parts.empty())
	{
		parts.emplace_back(std::format("{:03d}ns", nanoseconds));
	}

	return std::format("{}", std::join(parts, " "));
}

inline auto ms_to_string(std::uint32_t p_ms) -> std::string
{
	constexpr std::uint32_t ms_per_sec	= 1000;
	constexpr std::uint32_t sec_per_min = 60;
	constexpr std::uint32_t min_per_hr	= 60;
	constexpr std::uint32_t hr_per_day	= 24;

	const std::uint32_t days	= p_ms / (ms_per_sec * sec_per_min * min_per_hr * hr_per_day);
	std::uint32_t remainder		= p_ms % (ms_per_sec * sec_per_min * min_per_hr * hr_per_day);

	const std::uint32_t hours	= remainder / (ms_per_sec * sec_per_min * min_per_hr);
	remainder					= remainder % (ms_per_sec * sec_per_min * min_per_hr);

	const std::uint32_t minutes = remainder / (ms_per_sec * sec_per_min);
	remainder					= remainder % (ms_per_sec * sec_per_min);

	const std::uint32_t seconds = remainder / ms_per_sec;
	const std::uint32_t ms		= remainder % ms_per_sec;

	std::vector<std::string> parts;
	parts.reserve(5);

	if (days > 0)
	{
		parts.emplace_back(std::format("{}d", days));
	}
	if (hours > 0)
	{
		parts.emplace_back(std::format("{}h", hours));
	}
	if (minutes > 0)
	{
		parts.emplace_back(std::format("{}min", minutes));
	}
	if (seconds > 0)
	{
		parts.emplace_back(std::format("{:03d}s", seconds));
	}
	if (ms > 0 || parts.empty())
	{
		parts.emplace_back(std::format("{:03d}ms", ms));
	}

	return std::format("{}", std::join(parts, " "));
}

#endif	  // UTILS_MANIP
