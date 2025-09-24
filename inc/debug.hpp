#pragma once
#ifndef DEBUG_HPP
	#define DEBUG_HPP

	#if defined(UTILS_VERBOSE)
		#include "format.hpp"
		#define UTILS_DEBUG_LOG(...)                                                                                                                 \
			do                                                                                                                                       \
			{                                                                                                                                        \
				std::print("[DEBUG] " __VA_ARGS__);                                                                                                  \
				std::print("\n");                                                                                                                    \
			} while (0)
	#else
		#define UTILS_DEBUG_LOG(...)                                                                                                                 \
			do                                                                                                                                       \
			{                                                                                                                                        \
			} while (0)
	#endif

#endif	  // DEBUG_HPP