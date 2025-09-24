// File: inc/utils/guid_generator.hpp

#pragma once
#ifndef GUID_GENERATOR_HPP
	#define GUID_GENERATOR_HPP

	#include <atomic>
	#include <cstdint>
	#include <random>

namespace utils
{
	class guid_generator
	{
	public:
		enum constants : std::uint32_t
		{
			COUNTER_BITS = 0xFFFF,
			RANDOM_BITS	 = 0xFFFF0000
		};

	private:
		static auto get_counter() -> std::atomic<std::uint32_t>&
		{
			static std::atomic<std::uint32_t> instance{1};
			return instance;
		}

		static auto get_random_device() -> std::random_device&
		{
			static std::random_device instance{};
			return instance;
		}

		static auto get_generator() -> std::mt19937&
		{
			static std::mt19937 instance{get_random_device()()};
			return instance;
		}

		static auto get_distribution() -> std::uniform_int_distribution<std::uint32_t>&
		{
			static std::uniform_int_distribution<std::uint32_t> instance{};
			return instance;
		}

	public:
		static auto generate() -> std::uint32_t
		{
			std::uint32_t counter_part = get_counter().fetch_add(1) & constants::COUNTER_BITS;
			std::uint32_t random_part  = get_distribution()(get_generator()) & constants::RANDOM_BITS;
			return counter_part | random_part;
		}

		static auto reset_counter() -> void { get_counter().store(0); }
	};
}	 // namespace utils

#endif