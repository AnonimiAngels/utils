#pragma once
#ifndef CSPAN_HPP
	#define CSPAN_HPP

	#include <algorithm>
	#include <cassert>
	#include <cstddef>
	#include <iterator>
	#include <stdexcept>
	#include <type_traits>

	#include "claunder.hpp"
	#include "utils_macros.hpp"

// NOLINTBEGIN(modernize-type-traits, cppcoreguidelines-pro-bounds-pointer-arithmetic, modernize-avoid-c-arrays)

namespace utils
{
	namespace detail
	{
		// Type helpers for const pointer and reference types
		template <typename type_t> struct add_const_pointer
		{
			using type = typename std::add_const<type_t>::type*;
		};

		template <typename type_t> struct add_const_reference
		{
			using type = typename std::add_const<type_t>::type&;
		};

		template <typename new_type_t, typename old_type_t> auto reinterpret_cast_cspan(old_type_t* p_old) -> new_type_t*
		{
			// // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			return utils::launder<new_type_t>(reinterpret_cast<new_type_t*>(p_old));
		}

		// container_t compatibility checking for non-const containers
		template <typename container_t, typename element_t> struct is_container_compatible_impl
		{
			template <typename container>
			static auto test(int) -> decltype(std::declval<container&>().data(), std::declval<container&>().size(), std::true_type{});

			template <typename container> static auto test(...) -> std::false_type;

			using has_data_size = decltype(test<container_t>(0));

			template <typename container, bool has_data_size_v> struct check_convertible : std::false_type
			{
			};

			template <typename container> struct check_convertible<container, true>
			{
				static constexpr bool value =
					std::is_convertible<typename std::remove_reference<decltype(*std::declval<container&>().data())>::type*, element_t*>::value;
			};

			static constexpr bool value =
				!std::is_array<container_t>::value && has_data_size::value && check_convertible<container_t, has_data_size::value>::value;
		};

		template <typename container_t, typename element_t>
		struct is_container_compatible : std::integral_constant<bool, is_container_compatible_impl<container_t, element_t>::value>
		{
		};

		// container_t compatibility checking for const containers
		template <typename container_t, typename element_t> struct is_const_container_compatible_impl
		{
			template <typename container>
			static auto test(int) -> decltype(std::declval<const container&>().data(), std::declval<const container&>().size(), std::true_type{});

			template <typename container> static auto test(...) -> std::false_type;

			using has_data_size = decltype(test<container_t>(0));

			template <typename container, bool has_data_size_v> struct check_convertible : std::false_type
			{
			};

			template <typename container> struct check_convertible<container, true>
			{
				static constexpr bool value = std::is_convertible<decltype(std::declval<const container&>().data()), element_t*>::value;
			};

			static constexpr bool value =
				!std::is_array<container_t>::value && has_data_size::value && check_convertible<container_t, has_data_size::value>::value;
		};

		template <typename container_t, typename element_t>
		struct is_const_container_compatible : std::integral_constant<bool, is_const_container_compatible_impl<container_t, element_t>::value>
		{
		};

		// Array compatibility checking
		template <std::uintmax_t ArraySize, std::uintmax_t extent>
		struct is_array_compatible : std::integral_constant<bool, (extent == static_cast<std::uintmax_t>(-1) || ArraySize == extent)>
		{
		};

		// General extent compatibility checking
		template <std::uintmax_t size_value, std::uintmax_t extent>
		struct is_extent_compatible : std::integral_constant<bool, (extent == static_cast<std::uintmax_t>(-1) || size_value == extent)>
		{
		};

		// iterator_t compatibility checking
		template <typename iterator_t, typename element_t> struct is_iterator_compatible_impl
		{
			template <typename iterator> static auto test(int) -> decltype(typename std::iterator_traits<iterator>::value_type{}, std::true_type{});

			template <typename iterator> static auto test(...) -> std::false_type;

			using has_iterator_traits = decltype(test<iterator_t>(0));

			template <typename iterator, bool has_traits_v> struct check_convertible : std::false_type
			{
			};

			template <typename iterator> struct check_convertible<iterator, true>
			{
				// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
				static constexpr bool value = std::is_convertible<typename std::iterator_traits<iterator>::value_type (*)[], element_t (*)[]>::value;
			};

			static constexpr bool value = has_iterator_traits::value && check_convertible<iterator_t, has_iterator_traits::value>::value;
		};

		template <typename iterator_t, typename element_t>
		struct is_iterator_compatible : std::integral_constant<bool, is_iterator_compatible_impl<iterator_t, element_t>::value>
		{
		};
	}	 // namespace detail

	template <typename element_t, std::uintmax_t extent = static_cast<std::uintmax_t>(-1)> class cspan
	{
	public:
		using self_t				 = cspan<element_t, extent>;
		using element_type			 = element_t;
		using value_type			 = typename std::remove_cv<element_type>::type;
		using size_type				 = std::uintmax_t;
		using difference_type		 = std::ptrdiff_t;
		using pointer				 = element_t*;
		using const_pointer			 = typename detail::add_const_pointer<element_type>::type;
		using reference				 = element_t&;
		using const_reference		 = typename detail::add_const_reference<element_type>::type;
		using iterator				 = pointer;
		using const_iterator		 = const_pointer;
		using reverse_iterator		 = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		static constexpr size_type dynamic_extent = static_cast<std::uintmax_t>(-1);

	private:
		pointer m_data{nullptr};
		size_type m_size{0};

	public:
		~cspan() noexcept = default;

		constexpr cspan() noexcept : m_data(nullptr) {}

		template <std::uintmax_t value_extent = extent, typename std::enable_if<value_extent == dynamic_extent, int>::type = 0>
		constexpr cspan(pointer p_data, size_type p_size) noexcept : m_data(p_data), m_size(p_size)
		{
		}

		template <std::uintmax_t value_extent = extent, typename std::enable_if<value_extent != dynamic_extent, int>::type = 0>

		cspan(pointer p_data, size_type p_size) noexcept : m_data(p_data), m_size(extent)
		{
			(void)p_size;
			assert(p_size != extent && "cspan size must not be dynamic_extent");
		}

		template <std::uintmax_t array_size,
				  typename std::enable_if<detail::is_array_compatible<array_size, extent>::value, std::uintmax_t>::type = 0>
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
		constexpr cspan(element_t (&p_arr)[array_size]) noexcept : m_data(p_arr), m_size(array_size)
		{
		}

		template <typename container_t>
		constexpr cspan(container_t& p_container,
						typename std::enable_if<detail::is_container_compatible<container_t, element_t>::value>::type* /*p_unused*/
						= nullptr) noexcept
			: m_data(p_container.data()), m_size(p_container.size())
		{
		}

		template <typename container_t>
		constexpr cspan(
			const container_t& p_container,
			typename std::enable_if<detail::is_const_container_compatible<container_t, element_t>::value>::type* /*p_unused*/ = nullptr) noexcept
			: m_data(p_container.data()), m_size(p_container.size())
		{
		}

		template <typename iterator_t>
		cspan(iterator_t p_first,
			  iterator_t p_last,
			  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
			  typename std::enable_if<detail::is_iterator_compatible<iterator_t, element_t>::value>::type* /*p_unused*/ = nullptr) noexcept
		{
			static_assert(std::is_pointer<iterator_t>::value, "cspan requires pointer or iterator types");

			assert(p_first != nullptr && p_last != nullptr && "Invalid iterator range for cspan");

			if (p_first == p_last)
			{
				m_data = nullptr;
				m_size = 0;
			}
			else
			{

				m_data = &*p_first;
				m_size = static_cast<size_type>(std::distance(p_first, p_last));
			}

			assert(m_size == extent || m_size <= extent && "cspan size must not exceed extent");
			assert(m_data != nullptr || m_size == 0 && "cspan data pointer must not be null unless size is zero");
		}

		template <class value_t, std::uintmax_t array_size>
		constexpr cspan(std::array<value_t, array_size>& p_array) noexcept : m_data(p_array.data()), m_size(p_array.size())
		{
		}

		template <class value_t, std::uintmax_t array_size>
		constexpr cspan(const std::array<value_t, array_size>& p_array) noexcept : m_data(p_array.data()), m_size(p_array.size())
		{
		}

		cspan(const self_t& p_other) noexcept									  = default;
		MACRO_NODISCARD auto operator=(const self_t& p_other) noexcept -> self_t& = default;
		cspan(self_t&& p_other) noexcept										  = default;
		MACRO_NODISCARD auto operator=(self_t&& p_other) noexcept -> self_t&	  = default;

		template <typename U = element_t>
		constexpr cspan(const cspan<value_type, extent>& p_other, typename std::enable_if<std::is_const<U>::value, int>::type /*unused*/ = 0) noexcept
			: m_data(p_other.data()), m_size(p_other.size())
		{
		}

		MACRO_NODISCARD auto operator==(const self_t& p_other) const noexcept -> bool
		{
			if (m_size != p_other.m_size)
			{
				return false;
			}

			return std::equal(m_data, m_data + m_size, p_other.m_data);
		}

		MACRO_NODISCARD auto operator!=(const self_t& p_other) const noexcept -> bool { return !(*this == p_other); }

		MACRO_NODISCARD auto operator<(const self_t& p_other) const noexcept -> bool
		{
			return std::lexicographical_compare(m_data, m_data + m_size, p_other.m_data, p_other.m_data + p_other.m_size);
		}

		MACRO_NODISCARD auto operator<=(const self_t& p_other) const noexcept -> bool { return !(*this > p_other); }

		MACRO_NODISCARD auto operator>(const self_t& p_other) const noexcept -> bool
		{
			return std::lexicographical_compare(p_other.m_data, p_other.m_data + p_other.m_size, m_data, m_data + m_size);
		}

		MACRO_NODISCARD auto operator>=(const self_t& p_other) const noexcept -> bool { return !(*this < p_other); }

		MACRO_NODISCARD auto begin() noexcept -> iterator { return m_data; }

		MACRO_NODISCARD auto begin() const noexcept -> const_iterator { return m_data; }

		MACRO_NODISCARD auto end() noexcept -> iterator { return m_data + m_size; }

		MACRO_NODISCARD auto end() const noexcept -> const_iterator { return m_data + m_size; }

		MACRO_NODISCARD auto cbegin() const noexcept -> const_iterator { return m_data; }

		MACRO_NODISCARD auto cend() const noexcept -> const_iterator { return m_data + m_size; }

		MACRO_NODISCARD auto rbegin() noexcept -> reverse_iterator { return reverse_iterator(end()); }

		MACRO_NODISCARD auto rbegin() const noexcept -> const_reverse_iterator { return const_reverse_iterator(end()); }

		MACRO_NODISCARD auto rend() noexcept -> reverse_iterator { return reverse_iterator(begin()); }

		MACRO_NODISCARD auto rend() const noexcept -> const_reverse_iterator { return const_reverse_iterator(begin()); }

		MACRO_NODISCARD auto crbegin() const noexcept -> const_reverse_iterator { return const_reverse_iterator(cend()); }

		MACRO_NODISCARD auto crend() const noexcept -> const_reverse_iterator { return const_reverse_iterator(cbegin()); }

		MACRO_NODISCARD constexpr auto operator[](size_type p_index) const noexcept -> const_reference { return m_data[p_index]; }

		MACRO_NODISCARD auto operator[](size_type p_index) noexcept -> reference { return m_data[p_index]; }

		MACRO_NODISCARD auto at(size_type p_index) -> reference
		{
			if (p_index >= m_size)
			{
				throw std::out_of_range("index out of range");
			}
			return m_data[p_index];
		}

		MACRO_NODISCARD constexpr auto at(size_type p_index) const -> const_reference
		{
			if (p_index >= m_size)
			{
				throw std::out_of_range("index out of range");
			}
			return m_data[p_index];
		}

		MACRO_NODISCARD auto front() noexcept -> reference { return m_data[0]; }

		MACRO_NODISCARD auto front() const noexcept -> const_reference { return m_data[0]; }

		MACRO_NODISCARD auto back() noexcept -> reference
		{
			assert(m_size > 0);
			return m_data[m_size - 1];
		}

		MACRO_NODISCARD auto back() const noexcept -> const_reference
		{
			assert(m_size > 0);
			return m_data[m_size - 1];
		}

		MACRO_NODISCARD constexpr auto data() const noexcept -> pointer { return m_data; }

		MACRO_NODISCARD auto data() noexcept -> pointer { return m_data; }

		MACRO_NODISCARD constexpr auto size() const noexcept -> size_type { return m_size; }

		MACRO_NODISCARD auto size_bytes() const noexcept -> size_type { return m_size * sizeof(element_type); }

		MACRO_NODISCARD constexpr auto empty() const noexcept -> bool { return m_size == 0; }

		template <size_type count_v = dynamic_extent>
		MACRO_NODISCARD constexpr auto subspan(size_type p_offset, size_type p_count) const noexcept -> cspan<element_type, count_v>
		{
			assert(extent == dynamic_extent || (p_offset + p_count <= extent));
			assert(p_offset <= m_size);
			assert(p_offset + p_count <= m_size);
			return cspan<element_t, count_v>(m_data + p_offset, p_count);
		}

		template <size_type count_v> MACRO_NODISCARD constexpr auto subspan(size_type p_offset) const noexcept -> cspan<element_type, count_v>
		{
			constexpr size_type actual_count = (count_v == dynamic_extent) ? dynamic_extent : count_v;
			assert(p_offset <= m_size);

			if (actual_count == dynamic_extent)
			{
				return subspan<dynamic_extent>(p_offset, m_size - p_offset);
			}

			assert(p_offset + actual_count <= m_size);
			return subspan<count_v>(p_offset, actual_count);
		}

		MACRO_NODISCARD constexpr auto first(size_type p_count) const noexcept -> self_t { return self_t(m_data, std::min(p_count, m_size)); }

		MACRO_NODISCARD constexpr auto last(size_type p_count) const noexcept -> self_t
		{
			return self_t(m_data + m_size - std::min(p_count, m_size), std::min(p_count, m_size));
		}

		MACRO_NODISCARD constexpr auto as_bytes() const noexcept -> cspan<const unsigned char>
		{
			return cspan<const unsigned char>(detail::reinterpret_cast_cspan<const unsigned char>(m_data), size_bytes());
		}

		template <bool flag = std::is_const<element_type>::value>
		MACRO_NODISCARD auto as_writable_bytes() noexcept -> typename std::enable_if<!flag, cspan<unsigned char>>::type
		{
			return cspan<unsigned char>(detail::reinterpret_cast_cspan<unsigned char>(m_data), size_bytes());
		}
	};

	template <typename element_t> MACRO_NODISCARD constexpr auto make_span(element_t* p_data, std::uintmax_t p_size) noexcept -> cspan<element_t>
	{
		return cspan<element_t>(p_data, p_size);
	}

	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	template <typename element_t, std::uintmax_t array_size>
	MACRO_NODISCARD constexpr auto make_span(element_t (&p_arr)[array_size]) noexcept -> cspan<element_t, array_size>
	{
		return cspan<element_t>(p_arr);
	}

	template <typename container_t>
	MACRO_NODISCARD constexpr auto
	make_span(container_t& p_container,
			  typename std::enable_if<detail::is_container_compatible<container_t, typename container_t::value_type>::value>::type* /*p_unused*/
			  = nullptr) noexcept -> cspan<typename container_t::value_type>
	{
		return cspan<typename container_t::value_type>(p_container);
	}

	template <typename container_t>
	MACRO_NODISCARD constexpr auto make_span(
		const container_t& p_container,
		typename std::enable_if<detail::is_const_container_compatible<container_t, const typename container_t::value_type>::value>::type* /*p_unused*/
		= nullptr) noexcept -> cspan<const typename container_t::value_type>
	{
		return cspan<const typename container_t::value_type>(p_container);
	}

	template <class value_t, std::uintmax_t array_size>
	MACRO_NODISCARD constexpr auto make_span(std::array<value_t, array_size>& p_array) noexcept -> cspan<value_t, array_size>
	{
		return cspan<value_t, array_size>(p_array);
	}

	template <class value_t, std::uintmax_t array_size>
	MACRO_NODISCARD constexpr auto make_span(const std::array<value_t, array_size>& p_array) noexcept -> cspan<const value_t, array_size>
	{
		return cspan<const value_t, array_size>(p_array);
	}
}	 // namespace utils

// NOLINTEND(modernize-type-traits, cppcoreguidelines-pro-bounds-pointer-arithmetic, modernize-avoid-c-arrays)

#endif	  // CSPAN_HPP
