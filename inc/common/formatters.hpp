#pragma once
#ifndef FORMATTERS_HPP
#define FORMATTERS_HPP

#include <format>
#include <SDL3/SDL.h>

// Add formatter for SDL_FRect and SDL_FPoint and SDL_Rect and SDL_Point

template <> struct std::formatter<SDL_FRect>
{
	constexpr static auto parse(format_parse_context& ctx) -> const char* { return ctx.begin(); }

	template <typename FormatContext> auto format(const SDL_FRect& in_data, FormatContext& ctx) const -> typename FormatContext::iterator
	{
		return format_to(ctx.out(), "{{x: {}, y: {}, w: {}, h: {}}}", in_data.x, in_data.y, in_data.w, in_data.h);
	}
};

template <> struct std::formatter<SDL_FPoint>
{
	constexpr static auto parse(format_parse_context& ctx) -> const char* { return ctx.begin(); }

	template <typename FormatContext> auto format(const SDL_FPoint& in_data, FormatContext& ctx) const -> typename FormatContext::iterator
	{
		return format_to(ctx.out(), "{{x: {}, y:{}}}", in_data.x, in_data.y);
	}
};

#endif // FORMATTERS_HPP
