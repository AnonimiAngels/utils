#pragma once
#ifndef FILESYSTEM_HPP
	#define FILESYSTEM_HPP

	#include "utils_macros.hpp"

	#if MACRO_CXX17_ENABLED
		#include <filesystem>
namespace fs = std::filesystem;
	#else
		#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
	#endif

	#include "format.hpp"

	#if HAS_FMT_FORMAT && !HAS_STD_FORMAT && !MACRO_CXX17_ENABLED

FMT_BEGIN_NAMESPACE

template <> struct formatter<fs::path>
{
	static constexpr auto parse(format_parse_context& p_ctx) -> decltype(p_ctx.begin()) { return p_ctx.begin(); }

	template <typename format_context> auto format(const fs::path& p_var, format_context& p_ctx) const -> decltype(p_ctx.out())
	{
		return std::format_to(p_ctx.out(), "{}", p_var.string());
	}
};

FMT_END_NAMESPACE
	#endif

#endif	  // FILESYSTEM_HPP
