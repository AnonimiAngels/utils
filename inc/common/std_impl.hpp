

#pragma once
#ifndef STD_IMPL_HPP
#define STD_IMPL_HPP

#include <memory>
#include <type_traits>
#include <algorithm>

#if __cplusplus >= 202002L

#include <format>
#include <print>

#else

#define FMT_HEADER_ONLY

#include "fmt/core.h"
#include "fmt/format.h"

namespace custom_std
{
	class string_view
	{
	  public:
		using iterator		 = const char*;
		using const_iterator = const char*;

		constexpr string_view() noexcept : m_data(nullptr), m_size(0) {}
		template <size_t N> constexpr string_view(const char (&in_data)[N]) noexcept : m_data(in_data), m_size(N - 1) {}
		constexpr string_view(const char* in_data, size_t in_size) noexcept : m_data(in_data), m_size(in_size) {}
		constexpr string_view(const string_view&) noexcept = default;

		string_view(const std::string& in_str) noexcept : m_data(in_str.data()), m_size(in_str.size()) {}
		string_view(const char* in_data) noexcept : m_data(in_data), m_size(strlen(in_data)) {}

		constexpr auto data() const noexcept -> const char* { return m_data; }
		constexpr auto size() const noexcept -> size_t { return m_size; }
		constexpr auto empty() const noexcept -> bool { return m_size == 0; }

		auto operator=(const string_view&) noexcept -> string_view& = default;

		auto operator=(const std::string& in_str) noexcept -> string_view&
		{
			m_data = in_str.data();
			m_size = in_str.size();
			return *this;
		}

		auto operator=(const char* in_data) noexcept -> string_view&
		{
			m_data = in_data;
			m_size = strlen(in_data);
			return *this;
		}

		operator std::string() const { return {m_data, m_size}; }
		operator fmt::string_view() const { return {m_data, m_size}; }

		constexpr auto operator[](size_t pos) const -> char { return (pos < m_size) ? m_data[pos] : throw std::out_of_range("Index out of range"); }

		constexpr auto starts_with(const string_view& prefix) const noexcept -> bool
		{
			return (prefix.size() <= m_size) && (std::memcmp(m_data, prefix.data(), prefix.size()) == 0);
		}

		constexpr auto ends_with(const string_view& suffix) const noexcept -> bool
		{
			return (suffix.size() <= m_size) && (std::memcmp(m_data + m_size - suffix.size(), suffix.data(), suffix.size()) == 0);
		}

		auto substr(size_t pos, size_t len = npos) const -> string_view
		{
			if (pos > m_size) { throw std::out_of_range("Position out of range"); }

			len = std::min(len, m_size - pos);
			return {m_data + pos, len};
		}

		static constexpr size_t npos = static_cast<size_t>(-1);

		// Comparisons
		friend auto operator==(const string_view& lhs, const string_view& rhs) noexcept -> bool
		{
			return (lhs.m_size == rhs.m_size) && (std::memcmp(lhs.m_data, rhs.m_data, lhs.m_size) == 0);
		}
		friend auto operator!=(const string_view& lhs, const string_view& rhs) noexcept -> bool { return !(lhs == rhs); }
		friend auto operator<(const string_view& lhs, const string_view& rhs) noexcept -> bool { return std::memcmp(lhs.m_data, rhs.m_data, std::min(lhs.m_size, rhs.m_size)) < 0; }
		friend auto operator<=(const string_view& lhs, const string_view& rhs) noexcept -> bool { return !(rhs < lhs); }
		friend auto operator>(const string_view& lhs, const string_view& rhs) noexcept -> bool { return rhs < lhs; }
		friend auto operator>=(const string_view& lhs, const string_view& rhs) noexcept -> bool { return !(lhs < rhs); }

		// Iterators
		constexpr auto begin() const noexcept -> const_iterator { return m_data; }
		constexpr auto end() const noexcept -> const_iterator { return m_data + m_size; }
		constexpr auto cbegin() const noexcept -> const_iterator { return m_data; }
		constexpr auto cend() const noexcept -> const_iterator { return m_data + m_size; }

	  private:
		const char* m_data;
		size_t m_size;
	};
} // namespace custom_std

FMT_BEGIN_NAMESPACE
using custom_string_view = custom_std::string_view;
template <> struct formatter<custom_string_view>
{
	constexpr static auto parse(format_parse_context& ctx) -> const char* { return ctx.begin(); }

	template <typename FormatContext> auto format(const custom_string_view& in_str, FormatContext& ctx) const -> typename FormatContext::iterator
	{
		return format_to(ctx.out(), "{}", in_str.data());
	}
};
FMT_END_NAMESPACE

#endif

namespace std
{
#if __cplusplus >= 201402L // C++14 and beyond
	using std::make_unique;

#define MACRO_MAYBE_UNUSED [[maybe_unused]]
#else

#if __cplusplus <= 201103L
	using custom_std::string_view;
	using fmt::format;
	using fmt::print;

#define MACRO_MAYBE_UNUSED
#endif

	namespace detail
	{
		template <class> struct is_unbounded_array : std::false_type
		{
		};

		template <class T> struct is_unbounded_array<T[]> : std::true_type
		{
		};

		template <class> struct is_bounded_array : std::false_type
		{
		};

		template <class T, std::size_t N> struct is_bounded_array<T[N]> : std::true_type
		{
		};
	} // namespace detail

	// For single objects
	template <class T, class... Args> auto make_unique(Args&&... args) -> typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
	{
		static_assert(!std::is_array<T>::value, "T must be a complete type");
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

	// For unbouded arrays
	template <class T> auto make_unique(std::size_t in_size) -> typename std::enable_if<detail::is_unbounded_array<T>::value, std::unique_ptr<T>>::type
	{
		static_assert(detail::is_unbounded_array<T>::value, "T must be an unbounded array");
		return std::unique_ptr<T>(new typename std::remove_extent<T>::type[in_size]());
	}

	// Erase if
	template <class forward_iter_t, class predicate_t> auto erase_if(forward_iter_t in_first, const forward_iter_t in_last, predicate_t in_predicate) -> size_t
	{
		size_t l_erased = 0;
		in_first		= std::find_if(in_first, in_last, in_predicate);
		if (in_first != in_last)
		{
			for (forward_iter_t iter = in_first; ++iter != in_last;)
			{
				if (!in_predicate(*iter))
				{
					*in_first++ = std::move(*iter);
					++l_erased;
				}
			}
		}

		return l_erased;
	}

	template <class container_t, class predicate_t> auto erase_if(container_t&& in_container, predicate_t in_predicate) -> size_t
	{
		return erase_if(in_container.begin(), in_container.end(), in_predicate);
	}

	template <typename container_t, typename value_t> auto remove_equals(container_t& in_container, const value_t& in_value) -> size_t
	{
		return erase_if(in_container.begin(), in_container.end(), [&in_value](const value_t& in_elem) -> bool { return in_elem == in_value; });
	}

	template <typename container_t, typename value_t> auto remove_equals(container_t* in_container, const value_t& in_value) -> size_t
	{
		return erase_if(in_container->begin(), in_container->end(), [&in_value](const value_t& in_elem) -> bool { return in_elem == in_value; });
	}

#endif

}; // namespace std

#endif // STD_IMPL_HPP
