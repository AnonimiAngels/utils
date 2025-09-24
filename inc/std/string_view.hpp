#pragma once
#ifndef STRING_VIEW_HPP
	#define STRING_VIEW_HPP

	#include "format.hpp"
	#include "utils_macros.hpp"

	#if MACRO_CXX17_ENABLED
		#include <string_view>
	#else

		#include <algorithm>
		#include <cstring>
		#include <iterator>
		#include <string>

namespace std
{
	class string_view
	{
	public:
		using self_t				 = string_view;
		using value_type			 = char;
		using pointer				 = const char*;
		using const_pointer			 = const char*;
		using reference				 = const char&;
		using const_reference		 = const char&;
		using const_iterator		 = const char*;
		using iterator				 = const_iterator;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using reverse_iterator		 = const_reverse_iterator;
		using size_type				 = std::size_t;
		using difference_type		 = std::ptrdiff_t;

		static constexpr std::size_t npos = static_cast<std::size_t>(-1);

	private:
		const char* m_data;
		std::size_t m_size;

	public:
		constexpr string_view() noexcept : m_data(nullptr), m_size(0) {}

		template <std::size_t N> constexpr string_view(const char (&p_data)[N]) noexcept : m_data(p_data), m_size(N - 1) {}

		constexpr string_view(const char* p_data, std::size_t p_size) noexcept : m_data(p_data), m_size(p_size) {}

		string_view(const std::string& p_str) noexcept : m_data(p_str.data()), m_size(p_str.size()) {}

		string_view(const char* p_data) noexcept : m_data(p_data), m_size(std::strlen(p_data)) {}

		constexpr string_view(const self_t&) noexcept	  = default;
		auto operator=(const self_t&) noexcept -> self_t& = default;

		auto operator=(const std::string&) -> self_t& = delete;
		auto operator=(const char*) -> self_t&		  = delete;

		MACRO_NODISCARD constexpr auto data() const noexcept -> const char* { return m_data; }

		MACRO_NODISCARD constexpr auto size() const noexcept -> std::size_t { return m_size; }

		MACRO_NODISCARD constexpr auto empty() const noexcept -> bool { return m_size == 0; }

		constexpr auto operator[](std::size_t p_pos) const -> char { return m_data[p_pos]; }

		MACRO_NODISCARD constexpr auto starts_with(const self_t& p_prefix) const noexcept -> bool
		{
			return (p_prefix.size() <= m_size) && (std::memcmp(m_data, p_prefix.data(), p_prefix.size()) == 0);
		}

		MACRO_NODISCARD constexpr auto ends_with(const self_t& p_suffix) const noexcept -> bool
		{
			return (p_suffix.size() <= m_size) && (std::memcmp(m_data + m_size - p_suffix.size(), p_suffix.data(), p_suffix.size()) == 0);
		}

		MACRO_NODISCARD auto substr(std::size_t p_pos, std::size_t p_len = npos) const noexcept -> self_t
		{
			p_len = (p_len == npos) ? m_size - p_pos : std::min(p_len, m_size - p_pos);
			return {m_data + p_pos, p_len};
		}

		MACRO_NODISCARD constexpr auto begin() const noexcept -> const_iterator { return m_data; }

		MACRO_NODISCARD constexpr auto end() const noexcept -> const_iterator { return m_data + m_size; }

		MACRO_NODISCARD constexpr auto cbegin() const noexcept -> const_iterator { return m_data; }

		MACRO_NODISCARD constexpr auto cend() const noexcept -> const_iterator { return m_data + m_size; }

		MACRO_NODISCARD auto rbegin() const noexcept -> const_reverse_iterator { return const_reverse_iterator(end()); }

		MACRO_NODISCARD auto rend() const noexcept -> const_reverse_iterator { return const_reverse_iterator(begin()); }

		MACRO_NODISCARD auto crbegin() const noexcept -> const_reverse_iterator { return const_reverse_iterator(end()); }

		MACRO_NODISCARD auto crend() const noexcept -> const_reverse_iterator { return const_reverse_iterator(begin()); }

		operator std::string() const { return {m_data, m_size}; }

		friend auto operator==(const self_t& p_lhs, const self_t& p_rhs) noexcept -> bool
		{
			return p_lhs.size() == p_rhs.size() && std::equal(p_lhs.begin(), p_lhs.end(), p_rhs.begin());
		}

		friend auto operator!=(const self_t& p_lhs, const self_t& p_rhs) noexcept -> bool { return !(p_lhs == p_rhs); }

		friend auto operator<(const self_t& p_lhs, const self_t& p_rhs) noexcept -> bool
		{
			return std::lexicographical_compare(p_lhs.begin(), p_lhs.end(), p_rhs.begin(), p_rhs.end());
		}

		friend auto operator<=(const self_t& p_lhs, const self_t& p_rhs) noexcept -> bool { return !(p_rhs < p_lhs); }

		friend auto operator>(const self_t& p_lhs, const self_t& p_rhs) noexcept -> bool { return p_rhs < p_lhs; }

		friend auto operator>=(const self_t& p_lhs, const self_t& p_rhs) noexcept -> bool { return !(p_lhs < p_rhs); }
	};
}	 // namespace std
	#endif

	#ifdef HAS_FMT_FORMAT
FMT_BEGIN_NAMESPACE

template <> struct formatter<std::string_view>
{
	static constexpr auto parse(format_parse_context& p_ctx) -> decltype(p_ctx.begin()) { return p_ctx.begin(); }

	template <typename format_context> auto format(const std::string_view& p_var, format_context& p_ctx) -> decltype(p_ctx.out())
	{
		return std::format_to(p_ctx.out(), "{}", p_var.cbegin());
	}
};

FMT_END_NAMESPACE
	#endif

#endif