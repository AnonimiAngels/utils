#pragma once
#ifndef BENCH_TIMER_HPP
	#define BENCH_TIMER_HPP

	#include <chrono>
	#include <cstdio>
	#include <string>

struct bench_timer
{
private:
	using time_point = std::chrono::time_point<std::chrono::steady_clock>;
	time_point m_start_time;
	std::string m_name;

public:
	explicit bench_timer(std::string p_name = "") : m_start_time(std::chrono::steady_clock::now()), m_name(std::move(p_name)) {}

	~bench_timer() { print_elapsed(); }

	auto elapsed() const -> std::chrono::duration<double> { return std::chrono::steady_clock::now() - m_start_time; }

	auto print_elapsed() const -> void
	{
		const long long ns_total = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed()).count();

		const long long seconds = ns_total / 1000000000;
		const long long millis	= (ns_total / 1000000) % 1000;
		const long long micros	= (ns_total / 1000) % 1000;
		const long long nanos	= ns_total % 1000;

		std::string parts;
		if (seconds > 0)
		{
			parts += std::to_string(seconds) + "s";
		}
		if (millis > 0)
		{
			parts += (parts.empty() ? "" : " ") + std::to_string(millis) + "ms";
		}
		if (micros > 0)
		{
			parts += (parts.empty() ? "" : " ") + std::to_string(micros) + "µs";
		}
		if (nanos > 0)
		{
			parts += (parts.empty() ? "" : " ") + std::to_string(nanos) + "ns";
		}
		if (parts.empty())
		{
			parts = "0ns";
		}

		printf("%s: %s\n", m_name.c_str(), parts.c_str());
		fflush(stdout);
	}
};

#endif	  // BENCH_TIMER_HPP
