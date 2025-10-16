#pragma once
#ifndef FORMAT_HPP
	#define FORMAT_HPP
	#include "utils_macros.hpp"
	#if defined(__has_include)
		#if !MACRO_CXX20_ENABLED
			#if __has_include(<fmt/format.h>)
				#include <fmt/chrono.h>
				#include <fmt/color.h>
				#include <fmt/format.h>
				#include <fmt/ostream.h>
				#include <fmt/ranges.h>
				#include <fmt/std.h>
				#include <fmt/xchar.h>
				#define HAS_FMT_FORMAT 1
			#endif
		#else
			#if __has_include(<fmt/format.h>)
				#include <fmt/chrono.h>
				#include <fmt/color.h>
				#include <fmt/format.h>
				#include <fmt/ostream.h>
				#include <fmt/ranges.h>
				#include <fmt/std.h>
				#include <fmt/xchar.h>
				#define HAS_FMT_FORMAT 1
			#endif
		#endif
	#endif
	#ifndef HAS_FMT_FORMAT
		#define HAS_FMT_FORMAT 0
	#endif
	#if MACRO_CXX20_ENABLED
		#if defined(__has_include)
			#if __has_include(<format>)
				#include <format>
				#define HAS_STD_FORMAT 1
			#endif
		#endif
	#endif
	#ifndef HAS_STD_FORMAT
		#define HAS_STD_FORMAT 0
	#endif
	#if MACRO_CXX23_ENABLED
		#if defined(__has_include)
			#if __has_include(<print>)
				#include <print>
				#define HAS_STD_PRINT 1
			#endif
		#endif
	#endif
	#ifndef HAS_STD_PRINT
		#define HAS_STD_PRINT 0
	#endif

// IWYU pragma: begin_exports
	#if HAS_FMT_FORMAT
namespace std
{
		#if !HAS_STD_FORMAT
	// IWYU pragma: export
	using fmt::format;
	// IWYU pragma: export
	using fmt::format_to;
	// IWYU pragma: export
	using fmt::format_to_n;
	// IWYU pragma: export
	using fmt::formatted_size;
	// IWYU pragma: export
	using fmt::make_format_args;
	// IWYU pragma: export
	using fmt::vformat;
		#endif
		#if !HAS_STD_PRINT
	// IWYU pragma: export
	using fmt::print;
	// IWYU pragma: export
	using fmt::println;
		#endif
	// IWYU pragma: export
	using fmt::join;
}	 // namespace std
	#endif
// IWYU pragma: end_exports

	#define HAS_PRINT_SUPPORT (HAS_STD_PRINT || HAS_FMT_FORMAT)
#endif	  // FORMAT_HPP