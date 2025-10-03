#ifndef RANGES_HPP
#define RANGES_HPP
#include <algorithm>
#include <functional>
#include <iterator>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>

namespace ranges
{
	template <typename type_t> struct remove_cvref
	{
		using type = typename std::remove_cv<typename std::remove_reference<type_t>::type>::type;
	};

	template <typename type_t> using remove_cvref_t = typename remove_cvref<type_t>::type;

	template <typename type_t, typename = void> struct is_range : std::false_type
	{
	};

	template <typename type_t>
	struct is_range<type_t, decltype(std::begin(std::declval<type_t&>()), std::end(std::declval<type_t&>()), void())> : std::true_type
	{
	};

	template <typename iter_t> using iter_value_t		 = typename std::iterator_traits<iter_t>::value_type;
	template <typename iter_t> using iter_reference_t	 = typename std::iterator_traits<iter_t>::reference;
	template <typename iter_t> using iter_difference_t	 = typename std::iterator_traits<iter_t>::difference_type;
	template <typename range_t> using range_iterator_t	 = decltype(std::begin(std::declval<range_t&>()));
	template <typename range_t> using range_value_t		 = iter_value_t<range_iterator_t<range_t>>;
	template <typename range_t> using range_reference_t	 = iter_reference_t<range_iterator_t<range_t>>;
	template <typename range_t> using range_difference_t = iter_difference_t<range_iterator_t<range_t>>;
	template <typename range_t> using range_sentinel_t	 = decltype(std::end(std::declval<range_t&>()));
	template <typename range_t> using range_size_t		 = decltype(std::declval<range_t&>().size());
	template <typename range_t> using iterator_t		 = range_iterator_t<range_t>;
	template <typename range_t> using sentinel_t		 = range_sentinel_t<range_t>;

	template <typename iter_t, typename func_t> struct in_fun_result
	{
		iter_t in;
		func_t fun;
	};

	template <typename iter1_t, typename iter2_t> struct in_in_result
	{
		iter1_t in1;
		iter2_t in2;
	};

	template <typename iter_t, typename out_t> struct in_out_result
	{
		iter_t in;
		out_t out;
	};

	template <typename iter1_t, typename iter2_t, typename out_t> struct in_in_out_result
	{
		iter1_t in1;
		iter2_t in2;
		out_t out;
	};

	template <typename iter_t1, typename iter_t2> struct min_max_result
	{
		iter_t1 min;
		iter_t2 max;
	};

	template <typename iter_t> class subrange
	{
		iter_t m_begin;
		iter_t m_end;

	public:
		using iterator		  = iter_t;
		using value_type	  = iter_value_t<iter_t>;
		using reference		  = iter_reference_t<iter_t>;
		using difference_type = iter_difference_t<iter_t>;
		subrange()			  = default;
		template <typename in_iter_t> subrange(in_iter_t, in_iter_t);

		inline subrange(iter_t p_begin, iter_t p_end) : m_begin(p_begin), m_end(p_end) {}

		inline auto begin() const -> iter_t { return m_begin; }

		inline auto end() const -> iter_t { return m_end; }

		inline auto empty() const -> bool { return m_begin == m_end; }
	};

	template <typename iter_t> inline auto make_subrange(iter_t p_begin, iter_t p_end) -> subrange<iter_t>
	{
		return subrange<iter_t>(p_begin, p_end);
	}

	template <typename range_t> class ref_view
	{
		range_t* m_range;

	public:
		ref_view() : m_range(nullptr) {}

		inline explicit ref_view(range_t& p_r) : m_range(&p_r) {}

		inline auto begin() const -> range_iterator_t<range_t> { return std::begin(*m_range); }

		inline auto end() const -> range_iterator_t<range_t> { return std::end(*m_range); }
	};

	template <typename range_t> class ref_view<const range_t>
	{
		const range_t* m_range;

	public:
		ref_view() : m_range(nullptr) {}

		inline explicit ref_view(const range_t& p_r) : m_range(&p_r) {}

		inline auto begin() const -> range_iterator_t<const range_t> { return std::begin(*m_range); }

		inline auto end() const -> range_iterator_t<const range_t> { return std::end(*m_range); }
	};

	template <typename type_t, typename = void> struct has_size : std::false_type
	{
	};

	template <typename type_t> struct has_size<type_t, decltype(std::declval<type_t&>().size(), void())> : std::true_type
	{
	};

	template <typename type_t> struct is_sized_range : std::integral_constant<bool, is_range<type_t>::value && has_size<type_t>::value>
	{
	};

	template <typename iter_t, typename = void> struct is_input_iterator : std::false_type
	{
	};

	template <typename iter_t>
	struct is_input_iterator<
		iter_t,
		typename std::enable_if<std::is_convertible<typename std::iterator_traits<iter_t>::iterator_category, std::input_iterator_tag>::value>::type>
		: std::true_type
	{
	};

	template <typename iter_t, typename = void> struct is_forward_iterator : std::false_type
	{
	};

	template <typename iter_t>
	struct is_forward_iterator<iter_t,
							   typename std::enable_if<std::is_convertible<typename std::iterator_traits<iter_t>::iterator_category,
																		   std::forward_iterator_tag>::value>::type> : std::true_type
	{
	};

	template <typename iter_t, typename = void> struct is_bidirectional_iterator : std::false_type
	{
	};

	template <typename iter_t>
	struct is_bidirectional_iterator<iter_t,
									 typename std::enable_if<std::is_convertible<typename std::iterator_traits<iter_t>::iterator_category,
																				 std::bidirectional_iterator_tag>::value>::type> : std::true_type
	{
	};

	template <typename iter_t, typename = void> struct is_random_access_iterator : std::false_type
	{
	};

	template <typename iter_t>
	struct is_random_access_iterator<iter_t,
									 typename std::enable_if<std::is_convertible<typename std::iterator_traits<iter_t>::iterator_category,
																				 std::random_access_iterator_tag>::value>::type> : std::true_type
	{
	};

	template <typename type_t>
	struct is_input_range : std::integral_constant<bool, is_range<type_t>::value && is_input_iterator<range_iterator_t<type_t>>::value>
	{
	};

	template <typename type_t>
	struct is_forward_range : std::integral_constant<bool, is_range<type_t>::value && is_forward_iterator<range_iterator_t<type_t>>::value>
	{
	};

	template <typename type_t>
	struct is_bidirectional_range
		: std::integral_constant<bool, is_range<type_t>::value && is_bidirectional_iterator<range_iterator_t<type_t>>::value>
	{
	};

	template <typename type_t>
	struct is_random_access_range
		: std::integral_constant<bool, is_range<type_t>::value && is_random_access_iterator<range_iterator_t<type_t>>::value>
	{
	};

	template <typename base_range_t> class common_view
	{
		base_range_t m_base;

	public:
		common_view() = default;

		inline explicit common_view(base_range_t p_base) : m_base(std::move(p_base)) {}

		inline auto begin() -> range_iterator_t<base_range_t> { return std::begin(m_base); }

		inline auto end() -> range_iterator_t<base_range_t> { return std::end(m_base); }

		inline auto begin() const -> range_iterator_t<const base_range_t> { return std::begin(m_base); }

		inline auto end() const -> range_iterator_t<const base_range_t> { return std::end(m_base); }

		inline auto base() const -> const base_range_t& { return m_base; }
	};

	template <typename base_range_t> class reverse_view
	{
		base_range_t m_base;

	public:
		reverse_view() = default;

		inline explicit reverse_view(base_range_t p_base) : m_base(std::move(p_base)) {}

		inline auto begin() -> std::reverse_iterator<range_iterator_t<base_range_t>>
		{
			return std::reverse_iterator<range_iterator_t<base_range_t>>(std::end(m_base));
		}

		inline auto end() -> std::reverse_iterator<range_iterator_t<base_range_t>>
		{
			return std::reverse_iterator<range_iterator_t<base_range_t>>(std::begin(m_base));
		}

		inline auto begin() const -> std::reverse_iterator<range_iterator_t<const base_range_t>>
		{
			return std::reverse_iterator<range_iterator_t<const base_range_t>>(std::end(m_base));
		}

		inline auto end() const -> std::reverse_iterator<range_iterator_t<const base_range_t>>
		{
			return std::reverse_iterator<range_iterator_t<const base_range_t>>(std::begin(m_base));
		}

		inline auto base() const -> const base_range_t& { return m_base; }
	};

	template <typename type_t, typename = void> struct is_view : std::false_type
	{
	};

	template <typename range_t> struct is_view<ref_view<range_t>> : std::true_type
	{
	};

	template <typename base_range_t> struct is_view<common_view<base_range_t>> : std::true_type
	{
	};

	template <typename base_range_t> struct is_view<reverse_view<base_range_t>> : std::true_type
	{
	};

	template <typename iter_t> struct is_view<subrange<iter_t>> : std::true_type
	{
	};

	template <typename type_t> struct is_borrowed_range : std::false_type
	{
	};

	template <typename iter_t> struct is_borrowed_range<subrange<iter_t>> : std::true_type
	{
	};

	template <typename type_t, std::size_t N> struct is_borrowed_range<type_t[N]> : std::true_type
	{
	};

	struct dangling
	{
		dangling() = default;

		template <typename... args_t> constexpr dangling(args_t&&...) {}
	};

	template <typename range_t>
	using borrowed_iterator_t =
		typename std::conditional<is_borrowed_range<remove_cvref_t<range_t>>::value, range_iterator_t<range_t>, dangling>::type;

	template <typename type_t> struct is_array : std::false_type
	{
	};

	template <typename type_t, std::size_t N> struct is_array<type_t[N]> : std::true_type
	{
	};

	template <typename type_t>
	struct is_movable : std::integral_constant<bool, std::is_move_constructible<type_t>::value && std::is_move_assignable<type_t>::value>
	{
	};

	template <typename range_t>
	using borrowed_subrange_t =
		typename std::conditional<is_borrowed_range<remove_cvref_t<range_t>>::value, subrange<range_iterator_t<range_t>>, dangling>::type;

	template <typename range_t> inline auto all(range_t& p_r) -> ref_view<range_t>
	{
		return ref_view<range_t>(p_r);
	}

	template <typename range_t> inline auto all(const range_t& p_r) -> ref_view<const range_t>
	{
		return ref_view<const range_t>(p_r);
	}

	template <typename value_t> class dynamic_array
	{
		value_t* m_data;
		std::size_t m_size;
		std::size_t m_capacity;

	public:
		using value_type	 = value_t;
		using iterator		 = value_t*;
		using const_iterator = const value_t*;

		dynamic_array() : m_data(nullptr), m_size(0), m_capacity(0) {}

		template <typename iter_t> dynamic_array(iter_t p_first, iter_t p_last)
		{
			m_size	   = std::distance(p_first, p_last);
			m_capacity = m_size;
			if (m_size > 0)
			{
				if (m_capacity > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(value_t))
				{
					m_data	   = nullptr;
					m_size	   = 0;
					m_capacity = 0;
					return;
				}
				m_data = new value_t[m_capacity];
				std::copy(p_first, p_last, m_data);
			}
			else
			{
				m_data = nullptr;
			}
		}

		dynamic_array(const dynamic_array& p_other) : m_data(nullptr), m_size(0), m_capacity(0)
		{
			if (p_other.m_size > 0)
			{
				m_size	   = p_other.m_size;
				m_capacity = p_other.m_capacity;
				if (m_capacity > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(value_t))
				{
					m_data	   = nullptr;
					m_size	   = 0;
					m_capacity = 0;
					return;
				}
				m_data = new value_t[m_capacity];
				std::copy(p_other.m_data, p_other.m_data + m_size, m_data);
			}
		}

		dynamic_array(dynamic_array&& p_other) : m_data(p_other.m_data), m_size(p_other.m_size), m_capacity(p_other.m_capacity)
		{
			p_other.m_data	   = nullptr;
			p_other.m_size	   = 0;
			p_other.m_capacity = 0;
		}

		dynamic_array& operator=(const dynamic_array& p_other)
		{
			if (this != &p_other)
			{
				delete[] m_data;
				if (p_other.m_size > 0)
				{
					m_size	   = p_other.m_size;
					m_capacity = p_other.m_capacity;
					if (m_capacity > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(value_t))
					{
						m_data	   = nullptr;
						m_size	   = 0;
						m_capacity = 0;
						return *this;
					}
					m_data = new value_t[m_capacity];
					std::copy(p_other.m_data, p_other.m_data + m_size, m_data);
				}
				else
				{
					m_data	   = nullptr;
					m_size	   = 0;
					m_capacity = 0;
				}
			}
			return *this;
		}

		dynamic_array& operator=(dynamic_array&& p_other)
		{
			if (this != &p_other)
			{
				delete[] m_data;
				m_data			   = p_other.m_data;
				m_size			   = p_other.m_size;
				m_capacity		   = p_other.m_capacity;
				p_other.m_data	   = nullptr;
				p_other.m_size	   = 0;
				p_other.m_capacity = 0;
			}
			return *this;
		}

		~dynamic_array() { delete[] m_data; }

		inline auto begin() -> iterator { return m_data; }

		inline auto end() -> iterator { return m_data + m_size; }

		inline auto begin() const -> const_iterator { return m_data; }

		inline auto end() const -> const_iterator { return m_data + m_size; }

		inline auto size() const -> std::size_t { return m_size; }

		inline auto empty() const -> bool { return m_size == 0; }
	};

	template <typename container_t> class sorted_view
	{
		container_t m_container;
		sorted_view(const sorted_view&)			   = delete;
		sorted_view& operator=(const sorted_view&) = delete;

	public:
		using value_type	 = typename container_t::value_type;
		using iterator		 = typename container_t::iterator;
		using const_iterator = typename container_t::const_iterator;
		sorted_view()		 = default;

		template <typename range_t> explicit sorted_view(range_t&& p_range) : m_container(std::begin(p_range), std::end(p_range))
		{
			std::sort(m_container.begin(), m_container.end());
		}

		sorted_view(sorted_view&& p_other)			  = default;
		sorted_view& operator=(sorted_view&& p_other) = default;

		inline auto begin() -> iterator { return m_container.begin(); }

		inline auto end() -> iterator { return m_container.end(); }

		inline auto begin() const -> const_iterator { return m_container.begin(); }

		inline auto end() const -> const_iterator { return m_container.end(); }

		inline auto size() const -> std::size_t { return m_container.size(); }

		inline auto empty() const -> bool { return m_container.empty(); }
	};

	template <typename iter_t, typename sentinel_t, typename pred_t> class filter_iterator
	{
		iter_t m_current;
		sentinel_t m_end;
		pred_t m_pred;

		inline auto satisfy() -> void
		{
			while (m_current != m_end && !m_pred(*m_current))
				++m_current;
		}

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type		= iter_value_t<iter_t>;
		using reference			= iter_reference_t<iter_t>;
		using pointer			= typename std::iterator_traits<iter_t>::pointer;
		using difference_type	= iter_difference_t<iter_t>;
		filter_iterator()		= default;

		inline filter_iterator(iter_t p_current, sentinel_t p_end, pred_t p_pred) : m_current(p_current), m_end(p_end), m_pred(p_pred) { satisfy(); }

		inline auto operator*() const -> reference { return *m_current; }

		inline auto operator->() const -> pointer { return &*m_current; }

		inline auto operator++() -> filter_iterator&
		{
			++m_current;
			satisfy();
			return *this;
		}

		inline auto operator++(int) -> filter_iterator
		{
			filter_iterator tmp = *this;
			++*this;
			return tmp;
		}

		inline auto operator==(const filter_iterator& p_other) const -> bool { return m_current == p_other.m_current; }

		inline auto operator!=(const filter_iterator& p_other) const -> bool { return m_current != p_other.m_current; }

		inline auto base() const -> iter_t { return m_current; }
	};

	template <typename range_t, typename pred_t> class filter_view
	{
		range_t m_base;
		pred_t m_pred;

	public:
		using iterator = filter_iterator<range_iterator_t<range_t>, range_iterator_t<range_t>, pred_t>;
		filter_view()  = default;

		inline filter_view(range_t p_base, pred_t p_pred) : m_base(p_base), m_pred(p_pred) {}

		inline auto begin() -> iterator { return iterator(std::begin(m_base), std::end(m_base), m_pred); }

		inline auto end() -> iterator { return iterator(std::end(m_base), std::end(m_base), m_pred); }

		inline auto begin() const -> filter_iterator<range_iterator_t<const range_t>, range_iterator_t<const range_t>, pred_t>
		{
			return filter_iterator<range_iterator_t<const range_t>, range_iterator_t<const range_t>, pred_t>(std::begin(m_base),
																											 std::end(m_base),
																											 m_pred);
		}

		inline auto end() const -> filter_iterator<range_iterator_t<const range_t>, range_iterator_t<const range_t>, pred_t>
		{
			return filter_iterator<range_iterator_t<const range_t>, range_iterator_t<const range_t>, pred_t>(std::end(m_base),
																											 std::end(m_base),
																											 m_pred);
		}
	};

	template <typename iter_t, typename func_t> class transform_iterator
	{
		iter_t m_current;
		func_t m_func;

	public:
		using iterator_category = typename std::iterator_traits<iter_t>::iterator_category;
		using value_type		= typename std::result_of<func_t(iter_reference_t<iter_t>)>::type;
		using reference			= value_type;
		using pointer			= void;
		using difference_type	= iter_difference_t<iter_t>;
		transform_iterator()	= default;

		inline transform_iterator(iter_t p_current, func_t p_func) : m_current(p_current), m_func(p_func) {}

		inline auto operator*() const -> reference { return m_func(*m_current); }

		inline auto operator++() -> transform_iterator&
		{
			++m_current;
			return *this;
		}

		inline auto operator++(int) -> transform_iterator
		{
			transform_iterator tmp = *this;
			++m_current;
			return tmp;
		}

		inline auto operator--() -> transform_iterator&
		{
			--m_current;
			return *this;
		}

		inline auto operator--(int) -> transform_iterator
		{
			transform_iterator tmp = *this;
			--m_current;
			return tmp;
		}

		inline auto operator+(difference_type p_n) const -> transform_iterator { return transform_iterator(m_current + p_n, m_func); }

		inline auto operator-(difference_type p_n) const -> transform_iterator { return transform_iterator(m_current - p_n, m_func); }

		inline auto operator-(const transform_iterator& p_other) const -> difference_type { return m_current - p_other.m_current; }

		inline auto operator==(const transform_iterator& p_other) const -> bool { return m_current == p_other.m_current; }

		inline auto operator!=(const transform_iterator& p_other) const -> bool { return m_current != p_other.m_current; }

		inline auto operator<(const transform_iterator& p_other) const -> bool { return m_current < p_other.m_current; }

		inline auto operator<=(const transform_iterator& p_other) const -> bool { return m_current <= p_other.m_current; }

		inline auto operator>(const transform_iterator& p_other) const -> bool { return m_current > p_other.m_current; }

		inline auto operator>=(const transform_iterator& p_other) const -> bool { return m_current >= p_other.m_current; }

		inline auto base() const -> iter_t { return m_current; }
	};

	template <typename iter_t> class enumerate_iterator
	{
		iter_t m_current;
		std::size_t m_index;

	public:
		using iterator_category = typename std::iterator_traits<iter_t>::iterator_category;
		using value_type		= std::pair<std::size_t, iter_reference_t<iter_t>>;
		using reference			= value_type;
		using pointer			= void;
		using difference_type	= iter_difference_t<iter_t>;
		enumerate_iterator()	= default;

		inline enumerate_iterator(iter_t p_current, std::size_t p_index = 0) : m_current(p_current), m_index(p_index) {}

		inline auto operator*() const -> reference { return {m_index, *m_current}; }

		inline auto operator++() -> enumerate_iterator&
		{
			++m_current;
			++m_index;
			return *this;
		}

		inline auto operator++(int) -> enumerate_iterator
		{
			enumerate_iterator tmp = *this;
			++*this;
			return tmp;
		}

		inline auto operator--() -> enumerate_iterator&
		{
			--m_current;
			--m_index;
			return *this;
		}

		inline auto operator--(int) -> enumerate_iterator
		{
			enumerate_iterator tmp = *this;
			--*this;
			return tmp;
		}

		inline auto operator+(difference_type p_n) const -> enumerate_iterator { return enumerate_iterator(m_current + p_n, m_index + p_n); }

		inline auto operator-(difference_type p_n) const -> enumerate_iterator { return enumerate_iterator(m_current - p_n, m_index - p_n); }

		inline auto operator-(const enumerate_iterator& p_other) const -> difference_type { return m_current - p_other.m_current; }

		inline auto operator==(const enumerate_iterator& p_other) const -> bool { return m_current == p_other.m_current; }

		inline auto operator!=(const enumerate_iterator& p_other) const -> bool { return m_current != p_other.m_current; }

		inline auto operator<(const enumerate_iterator& p_other) const -> bool { return m_current < p_other.m_current; }

		inline auto operator<=(const enumerate_iterator& p_other) const -> bool { return m_current <= p_other.m_current; }

		inline auto operator>(const enumerate_iterator& p_other) const -> bool { return m_current > p_other.m_current; }

		inline auto operator>=(const enumerate_iterator& p_other) const -> bool { return m_current >= p_other.m_current; }

		inline auto base() const -> iter_t { return m_current; }
	};

	template <typename range_t> class enumerate_view
	{
		range_t m_base;

	public:
		using iterator	 = enumerate_iterator<range_iterator_t<range_t>>;
		enumerate_view() = default;

		inline explicit enumerate_view(range_t p_base) : m_base(p_base) {}

		inline auto begin() -> iterator { return iterator(std::begin(m_base), 0); }

		inline auto end() -> iterator
		{
			auto last = std::end(m_base);
			auto dist = std::distance(std::begin(m_base), last);
			return iterator(last, static_cast<std::size_t>(dist));
		}

		inline auto begin() const -> enumerate_iterator<range_iterator_t<const range_t>>
		{
			return enumerate_iterator<range_iterator_t<const range_t>>(std::begin(m_base), 0);
		}

		inline auto end() const -> enumerate_iterator<range_iterator_t<const range_t>>
		{
			auto last = std::end(m_base);
			auto dist = std::distance(std::begin(m_base), last);
			return enumerate_iterator<range_iterator_t<const range_t>>(last, static_cast<std::size_t>(dist));
		}
	};

	template <typename range_t, typename func_t> class transform_view
	{
	private:
		range_t m_base;
		func_t m_func;

	public:
		using iterator	 = transform_iterator<range_iterator_t<range_t>, func_t>;
		transform_view() = default;

		inline transform_view(range_t p_base, func_t p_func) : m_base(p_base), m_func(p_func) {}

		inline auto begin() -> iterator { return iterator(std::begin(m_base), m_func); }

		inline auto end() -> iterator { return iterator(std::end(m_base), m_func); }

		inline auto begin() const -> transform_iterator<range_iterator_t<const range_t>, func_t>
		{
			return transform_iterator<range_iterator_t<const range_t>, func_t>(std::begin(m_base), m_func);
		}

		inline auto end() const -> transform_iterator<range_iterator_t<const range_t>, func_t>
		{
			return transform_iterator<range_iterator_t<const range_t>, func_t>(std::end(m_base), m_func);
		}
	};

	template <typename iter_t, typename sentinel_t> class unique_iterator
	{
		iter_t m_current;
		sentinel_t m_end;

		inline auto skip_duplicates() -> void
		{
			if (m_current == m_end)
			{
				return;
			}

			iter_t next = m_current;
			++next;

			while (next != m_end && *m_current == *next)
			{
				++next;
			}
			// Move to the first occurrence of the next different value
			m_current = next;
		}

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type		= iter_value_t<iter_t>;
		using reference			= iter_reference_t<iter_t>;
		using pointer			= typename std::iterator_traits<iter_t>::pointer;
		using difference_type	= iter_difference_t<iter_t>;
		unique_iterator()		= default;

		inline unique_iterator(iter_t p_current, sentinel_t p_end) : m_current(p_current), m_end(p_end) {}

		inline auto operator*() const -> reference { return *m_current; }

		inline auto operator->() const -> pointer { return &*m_current; }

		inline auto operator++() -> unique_iterator&
		{
			++m_current;
			skip_duplicates();
			return *this;
		}

		inline auto operator++(int) -> unique_iterator
		{
			unique_iterator tmp = *this;
			++*this;
			return tmp;
		}

		inline auto operator==(const unique_iterator& p_other) const -> bool { return m_current == p_other.m_current; }

		inline auto operator!=(const unique_iterator& p_other) const -> bool { return m_current != p_other.m_current; }

		inline auto base() const -> iter_t { return m_current; }
	};

	template <typename range_t> class unique_view
	{
		range_t m_base;

	public:
		using iterator		  = unique_iterator<range_iterator_t<range_t>, range_sentinel_t<range_t>>;
		using sentinel		  = iterator;
		using const_iterator  = unique_iterator<range_iterator_t<const range_t>, range_sentinel_t<const range_t>>;
		using const_sentinel  = const_iterator;
		using value_type	  = range_value_t<range_t>;
		using reference		  = range_reference_t<range_t>;
		using difference_type = range_difference_t<range_t>;
		unique_view()		  = default;

		inline explicit unique_view(range_t p_base) : m_base(p_base) {}

		inline auto begin() -> iterator { return iterator(std::begin(m_base), std::end(m_base)); }

		inline auto end() -> iterator { return iterator(std::end(m_base), std::end(m_base)); }

		inline auto begin() const -> unique_iterator<range_iterator_t<const range_t>, range_sentinel_t<const range_t>>
		{
			return unique_iterator<range_iterator_t<const range_t>, range_sentinel_t<const range_t>>(std::begin(m_base), std::end(m_base));
		}

		inline auto end() const -> unique_iterator<range_iterator_t<const range_t>, range_sentinel_t<const range_t>>
		{
			return unique_iterator<range_iterator_t<const range_t>, range_sentinel_t<const range_t>>(std::end(m_base), std::end(m_base));
		}
	};

	template <typename range_t> class take_view
	{
		range_t m_base;
		std::size_t m_count;

		template <typename iter_t> class take_iterator
		{
			iter_t m_current;
			std::size_t m_count;
			std::size_t m_pos;

		public:
			using iterator_category = typename std::iterator_traits<iter_t>::iterator_category;
			using value_type		= iter_value_t<iter_t>;
			using reference			= iter_reference_t<iter_t>;
			using pointer			= typename std::iterator_traits<iter_t>::pointer;
			using difference_type	= iter_difference_t<iter_t>;

			take_iterator() : m_count(0), m_pos(0) {}

			inline take_iterator(iter_t p_current, std::size_t p_count, std::size_t p_pos = 0) : m_current(p_current), m_count(p_count), m_pos(p_pos)
			{
			}

			inline auto operator*() const -> reference { return *m_current; }

			inline auto operator->() const -> pointer { return &*m_current; }

			inline auto operator++() -> take_iterator&
			{
				++m_current;
				++m_pos;
				return *this;
			}

			inline auto operator++(int) -> take_iterator
			{
				take_iterator tmp = *this;
				++*this;
				return tmp;
			}

			inline auto operator==(const take_iterator& p_other) const -> bool { return m_pos == p_other.m_pos || m_current == p_other.m_current; }

			inline auto operator!=(const take_iterator& p_other) const -> bool { return !(*this == p_other); }
		};

	public:
		using iterator = take_iterator<range_iterator_t<range_t>>;

		take_view() : m_count(0) {}

		inline take_view(range_t p_base, std::size_t p_count) : m_base(p_base), m_count(p_count) {}

		inline auto begin() -> iterator { return iterator(std::begin(m_base), m_count); }

		inline auto end() -> iterator
		{
			auto it				= std::begin(m_base);
			auto last			= std::end(m_base);
			std::size_t idx_for = 0;
			while (idx_for < m_count && it != last)
			{
				++it;
				++idx_for;
			}
			return iterator(it, m_count, idx_for);
		}

		inline auto begin() const -> take_iterator<range_iterator_t<const range_t>>
		{
			return take_iterator<range_iterator_t<const range_t>>(std::begin(m_base), m_count);
		}

		inline auto end() const -> take_iterator<range_iterator_t<const range_t>>
		{
			auto it				= std::begin(m_base);
			auto last			= std::end(m_base);
			std::size_t idx_for = 0;
			while (idx_for < m_count && it != last)
			{
				++it;
				++idx_for;
			}
			return take_iterator<range_iterator_t<const range_t>>(it, m_count, idx_for);
		}
	};

	template <typename range_t> class drop_view
	{
		range_t m_base;
		std::size_t m_count;

	public:
		drop_view() : m_count(0) {}

		inline drop_view(range_t p_base, std::size_t p_count) : m_base(p_base), m_count(p_count) {}

		inline auto begin() -> range_iterator_t<range_t>
		{
			auto it				= std::begin(m_base);
			auto last			= std::end(m_base);
			std::size_t idx_for = 0;
			while (idx_for < m_count && it != last)
			{
				++it;
				++idx_for;
			}
			return it;
		}

		inline auto end() -> range_iterator_t<range_t> { return std::end(m_base); }

		inline auto begin() const -> range_iterator_t<const range_t>
		{
			auto it				= std::begin(m_base);
			auto last			= std::end(m_base);
			std::size_t idx_for = 0;
			while (idx_for < m_count && it != last)
			{
				++it;
				++idx_for;
			}
			return it;
		}

		inline auto end() const -> range_iterator_t<const range_t> { return std::end(m_base); }
	};

	template <typename type_t> class single_view
	{
		type_t m_value;

	public:
		single_view() = default;

		inline explicit single_view(const type_t& p_value) : m_value(p_value) {}

		inline explicit single_view(type_t&& p_value) : m_value(std::move(p_value)) {}

		inline auto begin() -> type_t* { return &m_value; }

		inline auto end() -> type_t* { return &m_value + 1; }

		inline auto begin() const -> const type_t* { return &m_value; }

		inline auto end() const -> const type_t* { return &m_value + 1; }
	};

	template <typename type_t> class empty_view
	{
	public:
		inline auto begin() -> type_t* { return nullptr; }

		inline auto end() -> type_t* { return nullptr; }

		inline auto begin() const -> const type_t* { return nullptr; }

		inline auto end() const -> const type_t* { return nullptr; }
	};

	template <typename type_t> class iota_view
	{
		type_t m_value;
		type_t m_bound;

		template <typename u> class iota_iterator
		{
			u m_value;

		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type		= u;
			using reference			= u;
			using pointer			= void;
			using difference_type	= std::ptrdiff_t;
			iota_iterator()			= default;

			inline explicit iota_iterator(u p_value) : m_value(p_value) {}

			inline auto operator*() const -> reference { return m_value; }

			inline auto operator++() -> iota_iterator&
			{
				++m_value;
				return *this;
			}

			inline auto operator++(int) -> iota_iterator
			{
				iota_iterator tmp = *this;
				++m_value;
				return tmp;
			}

			inline auto operator--() -> iota_iterator&
			{
				--m_value;
				return *this;
			}

			inline auto operator--(int) -> iota_iterator
			{
				iota_iterator tmp = *this;
				--m_value;
				return tmp;
			}

			inline auto operator+(difference_type p_n) const -> iota_iterator { return iota_iterator(m_value + p_n); }

			inline auto operator-(difference_type p_n) const -> iota_iterator { return iota_iterator(m_value - p_n); }

			inline auto operator-(const iota_iterator& p_other) const -> difference_type { return m_value - p_other.m_value; }

			inline auto operator==(const iota_iterator& p_other) const -> bool { return m_value == p_other.m_value; }

			inline auto operator!=(const iota_iterator& p_other) const -> bool { return m_value != p_other.m_value; }

			inline auto operator<(const iota_iterator& p_other) const -> bool { return m_value < p_other.m_value; }

			inline auto operator<=(const iota_iterator& p_other) const -> bool { return m_value <= p_other.m_value; }

			inline auto operator>(const iota_iterator& p_other) const -> bool { return m_value > p_other.m_value; }

			inline auto operator>=(const iota_iterator& p_other) const -> bool { return m_value >= p_other.m_value; }
		};

	public:
		using iterator = iota_iterator<type_t>;
		iota_view()	   = default;

		inline explicit iota_view(type_t p_value) : m_value(p_value), m_bound(std::numeric_limits<type_t>::max()) {}

		inline iota_view(type_t p_value, type_t p_bound) : m_value(p_value), m_bound(p_bound) {}

		inline auto begin() const -> iterator { return iterator(m_value); }

		inline auto end() const -> iterator { return iterator(m_bound); }
	};

	template <typename range_t> class owning_view
	{
		range_t m_range;

	public:
		owning_view() = default;

		inline explicit owning_view(range_t p_range) : m_range(std::move(p_range)) {}

		inline auto begin() -> decltype(std::begin(m_range)) { return std::begin(m_range); }

		inline auto end() -> decltype(std::end(m_range)) { return std::end(m_range); }

		inline auto begin() const -> decltype(std::begin(m_range)) { return std::begin(m_range); }

		inline auto end() const -> decltype(std::end(m_range)) { return std::end(m_range); }

		inline auto base() const -> const range_t& { return m_range; }
	};

	template <typename range_t> struct is_view<owning_view<range_t>> : std::true_type
	{
	};

	template <typename type_t> class repeat_iterator
	{
		const type_t* m_value_ptr;

	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type		= type_t;
		using reference			= const type_t&;
		using pointer			= const type_t*;
		using difference_type	= std::ptrdiff_t;

		repeat_iterator() : m_value_ptr(nullptr) {}

		inline explicit repeat_iterator(const type_t* p_value_ptr) : m_value_ptr(p_value_ptr) {}

		inline auto operator*() const -> reference { return *m_value_ptr; }

		inline auto operator->() const -> pointer { return m_value_ptr; }

		inline auto operator++() -> repeat_iterator& { return *this; }

		inline auto operator++(int) -> repeat_iterator { return *this; }

		inline auto operator--() -> repeat_iterator& { return *this; }

		inline auto operator--(int) -> repeat_iterator { return *this; }

		inline auto operator+(difference_type) const -> repeat_iterator { return *this; }

		inline auto operator-(difference_type) const -> repeat_iterator { return *this; }

		inline auto operator-(const repeat_iterator&) const -> difference_type { return 0; }

		inline auto operator==(const repeat_iterator& p_other) const -> bool { return m_value_ptr == p_other.m_value_ptr; }

		inline auto operator!=(const repeat_iterator& p_other) const -> bool { return m_value_ptr != p_other.m_value_ptr; }

		inline auto operator<(const repeat_iterator&) const -> bool { return false; }

		inline auto operator<=(const repeat_iterator&) const -> bool { return true; }

		inline auto operator>(const repeat_iterator&) const -> bool { return false; }

		inline auto operator>=(const repeat_iterator&) const -> bool { return true; }
	};

	template <typename type_t> class repeat_view
	{
		type_t m_value;

	public:
		using iterator = repeat_iterator<type_t>;
		repeat_view()  = default;

		inline explicit repeat_view(const type_t& p_value) : m_value(p_value) {}

		inline explicit repeat_view(type_t&& p_value) : m_value(std::move(p_value)) {}

		inline auto begin() const -> iterator { return iterator(&m_value); }

		inline auto end() const -> iterator { return iterator(nullptr); }
	};

	template <typename iter_t, typename sentinel_t, typename pred_t> class take_while_iterator
	{
		iter_t m_current;
		sentinel_t m_end;
		pred_t m_pred;
		bool m_at_end;

		inline auto check_end() -> void { m_at_end = (m_current == m_end) || !m_pred(*m_current); }

	public:
		using iterator_category = typename std::iterator_traits<iter_t>::iterator_category;
		using value_type		= iter_value_t<iter_t>;
		using reference			= iter_reference_t<iter_t>;
		using pointer			= typename std::iterator_traits<iter_t>::pointer;
		using difference_type	= iter_difference_t<iter_t>;

		take_while_iterator() : m_at_end(true) {}

		inline take_while_iterator(iter_t p_current, sentinel_t p_end, pred_t p_pred)
			: m_current(p_current), m_end(p_end), m_pred(p_pred), m_at_end(false)
		{
			if (m_current != m_end)
			{
				check_end();
			}
			else
			{
				m_at_end = true;
			}
		}

		inline auto operator*() const -> reference { return *m_current; }

		inline auto operator->() const -> pointer { return &*m_current; }

		inline auto operator++() -> take_while_iterator&
		{
			if (!m_at_end)
			{
				++m_current;
				check_end();
			}
			return *this;
		}

		inline auto operator++(int) -> take_while_iterator
		{
			take_while_iterator tmp = *this;
			++*this;
			return tmp;
		}

		inline auto operator==(const take_while_iterator& p_other) const -> bool
		{
			return (m_at_end && p_other.m_at_end) || (!m_at_end && !p_other.m_at_end && m_current == p_other.m_current);
		}

		inline auto operator!=(const take_while_iterator& p_other) const -> bool { return !(*this == p_other); }

		inline auto base() const -> iter_t { return m_current; }
	};

	template <typename range_t, typename pred_t> class take_while_view
	{
		range_t m_base;
		pred_t m_pred;

	public:
		using iterator	  = take_while_iterator<range_iterator_t<range_t>, decltype(std::end(std::declval<range_t&>())), pred_t>;
		take_while_view() = default;

		inline take_while_view(range_t p_base, pred_t p_pred) : m_base(p_base), m_pred(p_pred) {}

		inline auto begin() -> iterator { return iterator(std::begin(m_base), std::end(m_base), m_pred); }

		inline auto end() -> iterator
		{
			auto end_iter = std::end(m_base);
			return iterator(end_iter, end_iter, m_pred);
		}

		inline auto begin() const -> take_while_iterator<range_iterator_t<const range_t>, decltype(std::end(std::declval<const range_t&>())), pred_t>
		{
			return take_while_iterator<range_iterator_t<const range_t>, decltype(std::end(std::declval<const range_t&>())), pred_t>(
				std::begin(m_base), std::end(m_base), m_pred);
		}

		inline auto end() const -> take_while_iterator<range_iterator_t<const range_t>, decltype(std::end(std::declval<const range_t&>())), pred_t>
		{
			auto end_iter = std::end(m_base);
			return take_while_iterator<range_iterator_t<const range_t>, decltype(std::end(std::declval<const range_t&>())), pred_t>(end_iter,
																																	end_iter,
																																	m_pred);
		}
	};

	template <typename range_t, typename pred_t> class drop_while_view
	{
		range_t m_base;
		pred_t m_pred;

	public:
		drop_while_view() = default;

		inline drop_while_view(range_t p_base, pred_t p_pred) : m_base(p_base), m_pred(p_pred) {}

		inline auto begin() -> range_iterator_t<range_t>
		{
			auto iter	  = std::begin(m_base);
			auto end_iter = std::end(m_base);
			while (iter != end_iter && m_pred(*iter))
			{
				++iter;
			}
			return iter;
		}

		inline auto end() -> decltype(std::end(m_base)) { return std::end(m_base); }

		inline auto begin() const -> range_iterator_t<const range_t>
		{
			auto iter	  = std::begin(m_base);
			auto end_iter = std::end(m_base);
			while (iter != end_iter && m_pred(*iter))
			{
				++iter;
			}
			return iter;
		}

		inline auto end() const -> decltype(std::end(m_base)) { return std::end(m_base); }
	};

	template <typename range_of_ranges_t> class join_iterator
	{
		using outer_iter_t	= range_iterator_t<range_of_ranges_t>;
		using inner_range_t = range_value_t<range_of_ranges_t>;
		using inner_iter_t	= range_iterator_t<inner_range_t>;
		outer_iter_t m_outer_current;
		outer_iter_t m_outer_end;
		inner_iter_t m_inner_current;
		inner_iter_t m_inner_end;
		bool m_at_end;

		inline auto satisfy() -> void
		{
			while (m_outer_current != m_outer_end)
			{
				m_inner_current = std::begin(*m_outer_current);
				m_inner_end		= std::end(*m_outer_current);
				if (m_inner_current != m_inner_end)
				{
					return;
				}
				++m_outer_current;
			}
			m_at_end = true;
		}

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type		= range_value_t<inner_range_t>;
		using reference			= range_reference_t<inner_range_t>;
		using pointer			= typename std::iterator_traits<inner_iter_t>::pointer;
		using difference_type	= range_difference_t<inner_range_t>;

		join_iterator() : m_at_end(true) {}

		inline join_iterator(outer_iter_t p_outer_current, outer_iter_t p_outer_end)
			: m_outer_current(p_outer_current), m_outer_end(p_outer_end), m_at_end(false)
		{
			if (m_outer_current == m_outer_end)
			{
				m_at_end = true;
			}
			else
			{
				satisfy();
			}
		}

		inline auto operator*() const -> reference { return *m_inner_current; }

		inline auto operator->() const -> pointer { return &*m_inner_current; }

		inline auto operator++() -> join_iterator&
		{
			if (!m_at_end)
			{
				++m_inner_current;
				if (m_inner_current == m_inner_end)
				{
					++m_outer_current;
					satisfy();
				}
			}
			return *this;
		}

		inline auto operator++(int) -> join_iterator
		{
			join_iterator tmp = *this;
			++*this;
			return tmp;
		}

		inline auto operator==(const join_iterator& p_other) const -> bool
		{
			if (m_at_end && p_other.m_at_end)
				return true;
			if (m_at_end || p_other.m_at_end)
				return false;
			return m_outer_current == p_other.m_outer_current && m_inner_current == p_other.m_inner_current;
		}

		inline auto operator!=(const join_iterator& p_other) const -> bool { return !(*this == p_other); }
	};

	template <typename range_of_ranges_t> class join_view
	{
		range_of_ranges_t m_base;

	public:
		using iterator = join_iterator<range_of_ranges_t>;
		join_view()	   = default;

		inline explicit join_view(range_of_ranges_t p_base) : m_base(p_base) {}

		inline auto begin() -> iterator { return iterator(std::begin(m_base), std::end(m_base)); }

		inline auto end() -> iterator
		{
			auto end_iter = std::end(m_base);
			return iterator(end_iter, end_iter);
		}

		inline auto begin() const -> join_iterator<const range_of_ranges_t>
		{
			return join_iterator<const range_of_ranges_t>(std::begin(m_base), std::end(m_base));
		}

		inline auto end() const -> join_iterator<const range_of_ranges_t>
		{
			auto end_iter = std::end(m_base);
			return join_iterator<const range_of_ranges_t>(end_iter, end_iter);
		}
	};

	template <typename range_t, typename delimiter_t> class join_with_view
	{
		range_t m_base;
		delimiter_t m_delimiter;

	public:
		join_with_view() = default;

		inline join_with_view(range_t p_base, delimiter_t p_delimiter) : m_base(p_base), m_delimiter(p_delimiter) {}

		template <typename string_t = std::string> inline auto to() const -> string_t
		{
			string_t result;
			auto iter	  = std::begin(m_base);
			auto end_iter = std::end(m_base);

			if (iter != end_iter)
			{
				result += *iter;
				++iter;
			}

			while (iter != end_iter)
			{
				result += m_delimiter;
				result += *iter;
				++iter;
			}

			return result;
		}

		inline operator std::string() const { return to<std::string>(); }
	};

	template <typename range_t, typename delimiter_t>
	inline auto join_with(range_t&& p_range, delimiter_t&& p_delimiter) -> join_with_view<decltype(all(std::forward<range_t>(p_range))), delimiter_t>
	{
		return join_with_view<decltype(all(std::forward<range_t>(p_range))), delimiter_t>(all(std::forward<range_t>(p_range)), p_delimiter);
	}

	namespace views
	{
		struct filter_adaptor
		{
			template <typename pred_t> struct partial
			{
				pred_t m_pred;

				inline explicit partial(pred_t p_pred) : m_pred(p_pred) {}

				template <typename range_t>
				inline auto operator()(range_t&& p_range) const -> filter_view<decltype(all(std::forward<range_t>(p_range))), pred_t>
				{
					return filter_view<decltype(all(std::forward<range_t>(p_range))), pred_t>(all(std::forward<range_t>(p_range)), m_pred);
				}
			};

			template <typename pred_t> inline auto operator()(pred_t p_pred) const -> partial<pred_t> { return partial<pred_t>(p_pred); }
		};

		struct transform_adaptor
		{
			template <typename func_t> struct partial
			{
				func_t m_func;

				inline explicit partial(func_t p_func) : m_func(p_func) {}

				template <typename range_t>
				inline auto operator()(range_t&& p_range) const -> transform_view<decltype(all(std::forward<range_t>(p_range))), func_t>
				{
					return transform_view<decltype(all(std::forward<range_t>(p_range))), func_t>(all(std::forward<range_t>(p_range)), m_func);
				}
			};

			template <typename func_t> inline auto operator()(func_t p_func) const -> partial<func_t> { return partial<func_t>(p_func); }
		};

		struct take_adaptor
		{
			struct partial
			{
				std::size_t m_count;

				inline explicit partial(std::size_t p_count) : m_count(p_count) {}

				template <typename range_t>
				inline auto operator()(range_t&& p_range) const -> take_view<decltype(all(std::forward<range_t>(p_range)))>
				{
					return take_view<decltype(all(std::forward<range_t>(p_range)))>(all(std::forward<range_t>(p_range)), m_count);
				}
			};

			inline auto operator()(std::size_t p_count) const -> partial { return partial(p_count); }
		};

		struct drop_adaptor
		{
			struct partial
			{
				std::size_t m_count;

				inline explicit partial(std::size_t p_count) : m_count(p_count) {}

				template <typename range_t>
				inline auto operator()(range_t&& p_range) const -> drop_view<decltype(all(std::forward<range_t>(p_range)))>
				{
					return drop_view<decltype(all(std::forward<range_t>(p_range)))>(all(std::forward<range_t>(p_range)), m_count);
				}
			};

			inline auto operator()(std::size_t p_count) const -> partial { return partial(p_count); }
		};

		struct enumerate_adaptor
		{
			struct partial
			{
				template <typename range_t>
				inline auto operator()(range_t&& p_range) const -> enumerate_view<decltype(all(std::forward<range_t>(p_range)))>
				{
					return enumerate_view<decltype(all(std::forward<range_t>(p_range)))>(all(std::forward<range_t>(p_range)));
				}
			};

			inline auto operator()() const -> partial { return partial{}; }
		};

		struct join_adaptor
		{
			struct partial
			{
				template <typename range_t>
				inline auto operator()(range_t&& p_range) const -> join_view<decltype(all(std::forward<range_t>(p_range)))>
				{
					return join_view<decltype(all(std::forward<range_t>(p_range)))>(all(std::forward<range_t>(p_range)));
				}
			};

			inline auto operator()() const -> partial { return partial{}; }
		};

		struct sort_adaptor
		{
			struct partial
			{
				template <typename range_t>
				inline auto operator()(range_t&& p_range) const -> sorted_view<dynamic_array<range_value_t<remove_cvref_t<range_t>>>>
				{
					return sorted_view<dynamic_array<range_value_t<remove_cvref_t<range_t>>>>(std::forward<range_t>(p_range));
				}
			};

			inline auto operator()() const -> partial { return partial{}; }
		};

		static const filter_adaptor filter{};
		static const transform_adaptor transform{};
		static const take_adaptor take{};
		static const drop_adaptor drop{};
		static const enumerate_adaptor enumerate{};
		static const join_adaptor join{};
		static const sort_adaptor sort{};

		struct join_with_adaptor
		{
			template <typename delimiter_t> struct partial
			{
				delimiter_t m_delimiter;

				inline explicit partial(delimiter_t p_delimiter) : m_delimiter(p_delimiter) {}

				template <typename range_t>
				inline auto operator()(range_t&& p_range) const -> join_with_view<decltype(all(std::forward<range_t>(p_range))), delimiter_t>
				{
					return join_with_view<decltype(all(std::forward<range_t>(p_range))), delimiter_t>(all(std::forward<range_t>(p_range)),
																									  m_delimiter);
				}
			};

			template <typename delimiter_t> inline auto operator()(delimiter_t p_delimiter) const -> partial<delimiter_t>
			{
				return partial<delimiter_t>(p_delimiter);
			}
		};

		static const join_with_adaptor join_with{};

		struct unique_adaptor
		{
			struct partial
			{
				template <typename range_t>
				inline auto operator()(range_t&& p_range) const -> unique_view<decltype(all(std::forward<range_t>(p_range)))>
				{
					return unique_view<decltype(all(std::forward<range_t>(p_range)))>(all(std::forward<range_t>(p_range)));
				}
			};

			auto operator()() const -> partial { return partial{}; }
		};

		template <typename container_t> struct to_partial
		{
			template <typename range_t> auto operator()(const range_t& p_range) const -> container_t
			{
				container_t result;
				result.insert(std::end(result), std::begin(p_range), std::end(p_range));
				return result;
			}
		};

		template <typename container_t> inline auto to() -> to_partial<container_t>
		{
			return to_partial<container_t>{};
		}

		static const unique_adaptor unique{};

		struct common_adaptor
		{
			struct partial
			{
				template <typename range_t> inline auto operator()(range_t&& p_range) const -> common_view<ref_view<remove_cvref_t<range_t>>>
				{
					return common_view<ref_view<remove_cvref_t<range_t>>>(all(p_range));
				}
			};

			inline auto operator()() const -> partial { return partial{}; }
		};

		struct reverse_adaptor
		{
			struct partial
			{
				template <typename range_t> inline auto operator()(range_t&& p_range) const -> reverse_view<ref_view<remove_cvref_t<range_t>>>
				{
					return reverse_view<ref_view<remove_cvref_t<range_t>>>(all(p_range));
				}
			};

			inline auto operator()() const -> partial { return partial{}; }
		};

		static const common_adaptor common{};
		static const reverse_adaptor reverse{};

		template <typename iter_t> inline auto counted(iter_t p_first, std::size_t p_count) -> subrange<iter_t>
		{
			auto p_last = p_first;
			for (std::size_t idx_for = 0; idx_for < p_count; ++idx_for)
			{
				++p_last;
			}
			return subrange<iter_t>(p_first, p_last);
		};

		template <typename type_t> inline auto single(type_t&& p_value) -> single_view<remove_cvref_t<type_t>>
		{
			return single_view<remove_cvref_t<type_t>>(std::forward<type_t>(p_value));
		}

		template <typename type_t> inline auto empty() -> empty_view<type_t>
		{
			return empty_view<type_t>();
		}

		template <typename type_t> inline auto iota(type_t p_value) -> iota_view<type_t>
		{
			return iota_view<type_t>(p_value);
		}

		template <typename type_t> inline auto iota(type_t p_value, type_t p_bound) -> iota_view<type_t>
		{
			return iota_view<type_t>(p_value, p_bound);
		}

		template <typename range_t> inline auto all(const range_t& p_range) -> ref_view<const range_t>
		{
			return ref_view<const range_t>(p_range);
		}

		template <typename range_t> inline auto all(range_t& p_range) -> ref_view<range_t>
		{
			return ref_view<range_t>(p_range);
		}
	}	 // namespace views

	template <typename container_t> inline auto end(const sorted_view<container_t>& p_view) -> typename sorted_view<container_t>::const_iterator
	{
		return p_view.end();
	}

	template <typename container_t> inline auto end(sorted_view<container_t>& p_view) -> typename sorted_view<container_t>::iterator
	{
		return p_view.end();
	}

	template <typename range_t> inline auto end(range_t&& p_range) -> decltype(std::end(std::forward<range_t>(p_range)))
	{
		return std::end(std::forward<range_t>(p_range));
	}

	template <typename input_iter, typename func_t> inline auto for_each(input_iter p_first, input_iter p_last, func_t p_func) -> func_t
	{
		for (; p_first != p_last; ++p_first)
			p_func(*p_first);
		return p_func;
	}

	template <typename range_t, typename func_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto for_each(range_t&& p_range, func_t p_func) -> func_t
	{
		return ranges::for_each(std::begin(p_range), std::end(p_range), p_func);
	}

	template <typename input_iter, typename output_iter, typename func_t>
	inline auto transform(input_iter p_first, input_iter p_last, output_iter p_result, func_t p_func) -> output_iter
	{
		for (; p_first != p_last; ++p_first, ++p_result)
			*p_result = p_func(*p_first);
		return p_result;
	}

	template <typename range_t, typename output_iter, typename func_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto transform(range_t&& p_range, output_iter p_result, func_t p_func) -> output_iter
	{
		return ranges::transform(std::begin(p_range), std::end(p_range), p_result, p_func);
	}

	template <typename input_iter, typename type_t> inline auto find(input_iter p_first, input_iter p_last, const type_t& p_value) -> input_iter
	{
		while (p_first != p_last)
		{
			if (*p_first == p_value)
			{
				return p_first;
			}
			++p_first;
		}
		return p_last;
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto find(range_t&& p_range, const type_t& p_value) -> range_iterator_t<range_t>
	{
		return ranges::find(std::begin(p_range), std::end(p_range), p_value);
	}

	template <typename input_iter, typename pred_t> inline auto find_if(input_iter p_first, input_iter p_last, pred_t p_pred) -> input_iter
	{
		while (p_first != p_last)
		{
			if (p_pred(*p_first))
			{
				return p_first;
			}
			++p_first;
		}
		return p_last;
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto find_if(range_t&& p_range, pred_t p_pred) -> range_iterator_t<range_t>
	{
		return ranges::find_if(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename input_iter, typename pred_t> inline auto all_of(input_iter p_first, input_iter p_last, pred_t p_pred) -> bool
	{
		while (p_first != p_last)
		{
			if (!p_pred(*p_first))
			{
				return false;
			}
			++p_first;
		}
		return true;
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto all_of(range_t&& p_range, pred_t p_pred) -> bool
	{
		return ranges::all_of(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename input_iter, typename pred_t> inline auto any_of(input_iter p_first, input_iter p_last, pred_t p_pred) -> bool
	{
		while (p_first != p_last)
		{
			if (p_pred(*p_first))
			{
				return true;
			}
			++p_first;
		}
		return false;
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto any_of(range_t&& p_range, pred_t p_pred) -> bool
	{
		return ranges::any_of(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename input_iter, typename pred_t> inline auto none_of(input_iter p_first, input_iter p_last, pred_t p_pred) -> bool
	{
		while (p_first != p_last)
		{
			if (p_pred(*p_first))
			{
				return false;
			}
			++p_first;
		}
		return true;
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto none_of(range_t&& p_range, pred_t p_pred) -> bool
	{
		return ranges::none_of(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename input_iter>
	inline auto count(input_iter p_first, input_iter p_last) -> typename std::iterator_traits<input_iter>::difference_type
	{
		return std::distance(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto count(range_t&& p_range) -> range_difference_t<range_t>
	{
		return ranges::count(std::begin(p_range), std::end(p_range));
	}

	template <typename input_iter, typename type_t>
	inline auto count_val(input_iter p_first, input_iter p_last, const type_t& p_value) -> typename std::iterator_traits<input_iter>::difference_type
	{
		typename std::iterator_traits<input_iter>::difference_type ret = 0;
		while (p_first != p_last)
		{
			if (*p_first == p_value)
			{
				++ret;
			}
			++p_first;
		}
		return ret;
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto count_val(range_t&& p_range, const type_t& p_value) -> range_difference_t<range_t>
	{
		return ranges::count_val(std::begin(p_range), std::end(p_range), p_value);
	}

	template <typename input_iter, typename pred_t>
	inline auto count_if(input_iter p_first, input_iter p_last, pred_t p_pred) -> typename std::iterator_traits<input_iter>::difference_type
	{
		typename std::iterator_traits<input_iter>::difference_type ret = 0;
		while (p_first != p_last)
		{
			if (p_pred(*p_first))
			{
				++ret;
			}
			++p_first;
		}
		return ret;
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto count_if(range_t&& p_range, pred_t p_pred) -> range_difference_t<range_t>
	{
		return ranges::count_if(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename input_iter, typename output_iter> inline auto copy(input_iter p_first, input_iter p_last, output_iter p_result) -> output_iter
	{
		while (p_first != p_last)
		{
			*p_result++ = std::move(*p_first);
			++p_first;
		}
		return p_result;
	}

	template <typename range_t, typename output_iter, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto copy(range_t&& p_range, output_iter p_result) -> output_iter
	{
		return ranges::copy(std::begin(p_range), std::end(p_range), p_result);
	}

	template <typename input_iter, typename output_iter, typename pred_t>
	inline auto copy_if(input_iter p_first, input_iter p_last, output_iter p_result, pred_t p_pred) -> output_iter
	{
		while (p_first != p_last)
		{
			if (p_pred(*p_first))
			{
				*p_result++ = std::move(*p_first);
			}
			++p_first;
		}
		return p_result;
	}

	template <typename range_t, typename output_iter, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto copy_if(range_t&& p_range, output_iter p_result, pred_t p_pred) -> output_iter
	{
		return ranges::copy_if(std::begin(p_range), std::end(p_range), p_result, p_pred);
	}

	template <typename forward_iter, typename type_t> inline auto fill(forward_iter p_first, forward_iter p_last, const type_t& p_value) -> void
	{
		while (p_first != p_last)
		{
			*p_first = p_value;
			++p_first;
		}
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto fill(range_t&& p_range, const type_t& p_value) -> void
	{
		ranges::fill(std::begin(p_range), std::end(p_range), p_value);
	}

	template <typename forward_iter, typename generator_t> inline auto generate(forward_iter p_first, forward_iter p_last, generator_t p_gen) -> void
	{
		while (p_first != p_last)
		{
			*p_first = p_gen();
			++p_first;
		}
	}

	template <typename range_t, typename generator_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto generate(range_t&& p_range, generator_t p_gen) -> void
	{
		ranges::generate(std::begin(p_range), std::end(p_range), p_gen);
	}

	template <typename forward_iter, typename pred_t> inline auto remove_if(forward_iter p_first, forward_iter p_last, pred_t p_pred) -> forward_iter
	{
		p_first = ranges::find_if(p_first, p_last, p_pred);
		if (p_first != p_last)
		{
			forward_iter idx_for_i = p_first;
			while (++idx_for_i != p_last)
			{
				if (!p_pred(*idx_for_i))
				{
					*p_first = std::move(*idx_for_i);
					++p_first;
				}
			}
		}
		return p_first;
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto remove_if(range_t&& p_range, pred_t p_pred) -> range_iterator_t<range_t>
	{
		return ranges::remove_if(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename forward_iter, typename type_t>
	inline auto remove(forward_iter p_first, forward_iter p_last, const type_t& p_value) -> forward_iter
	{
		return ranges::remove_if(p_first,
								 p_last,
								 [&p_value](const typename std::iterator_traits<forward_iter>::reference p_ref) { return p_ref == p_value; });
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto remove(range_t&& p_range, const type_t& p_value) -> range_iterator_t<range_t>
	{
		return ranges::remove(std::begin(p_range), std::end(p_range), p_value);
	}

	template <typename bidirectional_iter> inline auto reverse(bidirectional_iter p_first, bidirectional_iter p_last) -> void
	{
		while ((p_first != p_last) && (p_first != --p_last))
		{
			std::iter_swap(p_first++, p_last);
		}
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type> inline auto reverse(range_t&& p_range) -> void
	{
		ranges::reverse(std::begin(p_range), std::end(p_range));
	}

	template <typename random_iter> inline auto sort(random_iter p_first, random_iter p_last) -> void
	{
		std::sort(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type> inline auto sort(range_t&& p_range) -> void
	{
		ranges::sort(std::begin(p_range), std::end(p_range));
	}

	template <typename random_iter, typename compare_t> inline auto sort(random_iter p_first, random_iter p_last, compare_t p_comp) -> void
	{
		std::sort(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto sort(range_t&& p_range, compare_t p_comp) -> void
	{
		ranges::sort(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename forward_iter> inline auto min_element(forward_iter p_first, forward_iter p_last) -> forward_iter
	{
		if (p_first == p_last)
		{
			return p_last;
		}
		forward_iter smallest = p_first;
		while (++p_first != p_last)
		{
			if (*p_first < *smallest)
			{
				smallest = p_first;
			}
		}
		return smallest;
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto min_element(range_t&& p_range) -> range_iterator_t<range_t>
	{
		return ranges::min_element(std::begin(p_range), std::end(p_range));
	}

	template <typename forward_iter, typename compare_t>
	inline auto min_element(forward_iter p_first, forward_iter p_last, compare_t p_comp) -> forward_iter
	{
		if (p_first == p_last)
			return p_last;
		forward_iter smallest = p_first;
		while (++p_first != p_last)
		{
			if (p_comp(*p_first, *smallest))
			{
				smallest = p_first;
			}
		}
		return smallest;
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto min_element(range_t&& p_range, compare_t p_comp) -> range_iterator_t<range_t>
	{
		return ranges::min_element(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename forward_iter> inline auto max_element(forward_iter p_first, forward_iter p_last) -> forward_iter
	{
		if (p_first == p_last)
			return p_last;
		forward_iter largest = p_first;
		while (++p_first != p_last)
		{
			if (*p_first > *largest)
			{
				largest = p_first;
			}
		}
		return largest;
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto max_element(range_t&& p_range) -> range_iterator_t<range_t>
	{
		return ranges::max_element(std::begin(p_range), std::end(p_range));
	}

	template <typename forward_iter, typename compare_t>
	inline auto max_element(forward_iter p_first, forward_iter p_last, compare_t p_comp) -> forward_iter
	{
		if (p_first == p_last)
			return p_last;
		forward_iter largest = p_first;
		while (++p_first != p_last)
		{
			if (p_comp(*largest, *p_first))
			{
				largest = p_first;
			}
		}
		return largest;
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto max_element(range_t&& p_range, compare_t p_comp) -> range_iterator_t<range_t>
	{
		return ranges::max_element(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename input_iter1, typename input_iter2> inline auto equal(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2) -> bool
	{
		while (p_first1 != p_last1)
		{
			if (!(*p_first1 == *p_first2))
			{
				return false;
			}
			++p_first1;
			++p_first2;
		}
		return true;
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto equal(range1_t&& p_range1, range2_t&& p_range2) -> bool
	{
		return ranges::equal(std::begin(p_range1), std::end(p_range1), std::begin(p_range2));
	}

	template <typename input_iter1, typename input_iter2, typename binary_pred_t>
	inline auto equal(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, binary_pred_t p_pred) -> bool
	{
		while (p_first1 != p_last1)
		{
			if (!p_pred(*p_first1, *p_first2))
			{
				return false;
			}
			++p_first1;
			++p_first2;
		}
		return true;
	}

	template <typename range1_t,
			  typename range2_t,
			  typename binary_pred_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto equal(range1_t&& p_range1, range2_t&& p_range2, binary_pred_t p_pred) -> bool
	{
		return ranges::equal(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), p_pred);
	}

	template <typename input_iter, typename type_t, typename binary_op_t>
	inline auto accumulate(input_iter p_first, input_iter p_last, type_t p_init, binary_op_t p_op) -> type_t
	{
		while (p_first != p_last)
		{
			p_init = p_op(std::move(p_init), *p_first);
			++p_first;
		}
		return p_init;
	}

	template <typename input_iter, typename type_t> inline auto accumulate(input_iter p_first, input_iter p_last, type_t p_init) -> type_t
	{
		return ranges::accumulate(p_first, p_last, p_init, std::plus<type_t>());
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto accumulate(range_t&& p_range, type_t p_init) -> type_t
	{
		return ranges::accumulate(std::begin(p_range), std::end(p_range), p_init);
	}

	template <typename range_t, typename type_t, typename binary_op_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto accumulate(range_t&& p_range, type_t p_init, binary_op_t p_op) -> type_t
	{
		return ranges::accumulate(std::begin(p_range), std::end(p_range), p_init, p_op);
	}

	template <typename input_iter, typename size_t, typename func_t>
	inline auto for_each_n(input_iter p_first, size_t p_n, func_t p_func) -> in_fun_result<input_iter, func_t>
	{
		if (p_n == 0)
		{
			return {p_first, p_func};
		}
		const input_iter end_iter{};
		if (p_first == end_iter)
		{
			return {p_first, p_func};
		}
		while (p_n-- && p_first != end_iter)
		{
			p_func(*p_first);
			++p_first;
		}
		return {p_first, p_func};
	}

	template <typename forward_iter> inline auto adjacent_find(forward_iter p_first, forward_iter p_last) -> forward_iter
	{
		if (p_first == p_last)
			return p_last;
		forward_iter next = p_first;
		while (++next != p_last)
		{
			if (*p_first == *next)
			{
				return p_first;
			}
			++p_first;
		}
		return p_last;
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto adjacent_find(range_t&& p_range) -> range_iterator_t<range_t>
	{
		return ranges::adjacent_find(std::begin(p_range), std::end(p_range));
	}

	template <typename forward_iter, typename binary_pred_t>
	inline auto adjacent_find(forward_iter p_first, forward_iter p_last, binary_pred_t p_pred) -> forward_iter
	{
		if (p_first == p_last)
			return p_last;
		forward_iter next = p_first;
		while (++next != p_last)
		{
			if (p_pred(*p_first, *next))
			{
				return p_first;
			}
			++p_first;
		}
		return p_last;
	}

	template <typename range_t, typename binary_pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto adjacent_find(range_t&& p_range, binary_pred_t p_pred) -> range_iterator_t<range_t>
	{
		return ranges::adjacent_find(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename input_iter1, typename input_iter2>
	inline auto mismatch(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2) -> in_in_result<input_iter1, input_iter2>
	{
		for (; p_first1 != p_last1 && !(*p_first1 == *p_first2); ++p_first1, ++p_first2)
			;
		return {p_first1, p_first2};
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto mismatch(range1_t&& p_range1, range2_t&& p_range2) -> in_in_result<range_iterator_t<range1_t>, range_iterator_t<range2_t>>
	{
		return ranges::mismatch(std::begin(p_range1), std::end(p_range1), std::begin(p_range2));
	}

	template <typename input_iter1, typename input_iter2, typename binary_pred_t>
	inline auto mismatch(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, binary_pred_t p_pred)
		-> in_in_result<input_iter1, input_iter2>
	{
		for (; p_first1 != p_last1 && p_pred(*p_first1, *p_first2); ++p_first1, ++p_first2)
			;
		return {p_first1, p_first2};
	}

	template <typename range1_t,
			  typename range2_t,
			  typename binary_pred_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto mismatch(range1_t&& p_range1, range2_t&& p_range2, binary_pred_t p_pred)
		-> in_in_result<range_iterator_t<range1_t>, range_iterator_t<range2_t>>
	{
		return ranges::mismatch(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), p_pred);
	}

	template <typename input_iter1, typename input_iter2>
	inline auto lexicographical_compare(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2) -> bool
	{
		while (p_first1 != p_last1 && p_first2 != p_last2)
		{
			if (*p_first1 < *p_first2)
				return true;
			if (*p_first2 < *p_first1)
				return false;
			++p_first1;
			++p_first2;
		}
		return p_first1 == p_last1 && p_first2 != p_last2;
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto lexicographical_compare(range1_t&& p_range1, range2_t&& p_range2) -> bool
	{
		return ranges::lexicographical_compare(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2));
	}

	template <typename input_iter1, typename input_iter2, typename compare_t>
	inline auto lexicographical_compare(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, compare_t p_comp)
		-> bool
	{
		while (p_first1 != p_last1 && p_first2 != p_last2)
		{
			if (p_comp(*p_first1, *p_first2))
				return true;
			if (p_comp(*p_first2, *p_first1))
				return false;
			++p_first1;
			++p_first2;
		}
		return p_first1 == p_last1 && p_first2 != p_last2;
	}

	template <typename range1_t,
			  typename range2_t,
			  typename compare_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto lexicographical_compare(range1_t&& p_range1, range2_t&& p_range2, compare_t p_comp) -> bool
	{
		return ranges::lexicographical_compare(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_comp);
	}

	template <typename input_iter, typename pred_t> inline auto find_if_not(input_iter p_first, input_iter p_last, pred_t p_pred) -> input_iter
	{
		while (p_first != p_last)
		{
			if (!p_pred(*p_first))
			{
				return p_first;
			}
			++p_first;
		}
		return p_last;
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto find_if_not(range_t&& p_range, pred_t p_pred) -> range_iterator_t<range_t>
	{
		return ranges::find_if_not(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename forward_iter1, typename forward_iter2>
	inline auto find_end(forward_iter1 p_first1, forward_iter1 p_last1, forward_iter2 p_first2, forward_iter2 p_last2) -> forward_iter1
	{
		if (p_first2 == p_last2)
			return p_last1;
		forward_iter1 result = p_last1;
		for (;;)
		{
			forward_iter1 new_result = std::search(p_first1, p_last1, p_first2, p_last2);
			if (new_result == p_last1)
				break;
			result	 = new_result;
			p_first1 = ++new_result;
		}
		return result;
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto find_end(range1_t&& p_range1, range2_t&& p_range2) -> range_iterator_t<range1_t>
	{
		return ranges::find_end(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2));
	}

	template <typename forward_iter1, typename forward_iter2, typename binary_pred_t>
	inline auto find_end(forward_iter1 p_first1, forward_iter1 p_last1, forward_iter2 p_first2, forward_iter2 p_last2, binary_pred_t p_pred)
		-> forward_iter1
	{
		if (p_first2 == p_last2)
			return p_last1;
		forward_iter1 result = p_last1;
		for (;;)
		{
			forward_iter1 new_result = std::search(p_first1, p_last1, p_first2, p_last2, p_pred);
			if (new_result == p_last1)
				break;
			result	 = new_result;
			p_first1 = ++new_result;
		}
		return result;
	}

	template <typename range1_t,
			  typename range2_t,
			  typename binary_pred_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto find_end(range1_t&& p_range1, range2_t&& p_range2, binary_pred_t p_pred) -> range_iterator_t<range1_t>
	{
		return ranges::find_end(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_pred);
	}

	template <typename input_iter, typename forward_iter>
	inline auto find_first_of(input_iter p_first1, input_iter p_last1, forward_iter p_first2, forward_iter p_last2) -> input_iter
	{
		for (; p_first1 != p_last1; ++p_first1)
			for (forward_iter it = p_first2; it != p_last2; ++it)
				if (*p_first1 == *it)
					return p_first1;
		return p_last1;
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto find_first_of(range1_t&& p_range1, range2_t&& p_range2) -> range_iterator_t<range1_t>
	{
		return ranges::find_first_of(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2));
	}

	template <typename input_iter, typename forward_iter, typename binary_pred_t>
	inline auto find_first_of(input_iter p_first1, input_iter p_last1, forward_iter p_first2, forward_iter p_last2, binary_pred_t p_pred)
		-> input_iter
	{
		for (; p_first1 != p_last1; ++p_first1)
			for (forward_iter it = p_first2; it != p_last2; ++it)
				if (p_pred(*p_first1, *it))
					return p_first1;
		return p_last1;
	}

	template <typename range1_t,
			  typename range2_t,
			  typename binary_pred_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto find_first_of(range1_t&& p_range1, range2_t&& p_range2, binary_pred_t p_pred) -> range_iterator_t<range1_t>
	{
		return ranges::find_first_of(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_pred);
	}

	template <typename forward_iter1, typename forward_iter2>
	inline auto search(forward_iter1 p_first1, forward_iter1 p_last1, forward_iter2 p_first2, forward_iter2 p_last2) -> forward_iter1
	{
		return std::search(p_first1, p_last1, p_first2, p_last2);
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto search(range1_t&& p_range1, range2_t&& p_range2) -> range_iterator_t<range1_t>
	{
		return ranges::search(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2));
	}

	template <typename forward_iter1, typename forward_iter2, typename binary_pred_t>
	inline auto search(forward_iter1 p_first1, forward_iter1 p_last1, forward_iter2 p_first2, forward_iter2 p_last2, binary_pred_t p_pred)
		-> forward_iter1
	{
		return std::search(p_first1, p_last1, p_first2, p_last2, p_pred);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename binary_pred_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto search(range1_t&& p_range1, range2_t&& p_range2, binary_pred_t p_pred) -> range_iterator_t<range1_t>
	{
		return ranges::search(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_pred);
	}

	template <typename forward_iter, typename size_t, typename type_t>
	inline auto search_n(forward_iter p_first, forward_iter p_last, size_t p_count, const type_t& p_value) -> forward_iter
	{
		return std::search_n(p_first, p_last, p_count, p_value);
	}

	template <typename range_t, typename size_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto search_n(range_t&& p_range, size_t p_count, const type_t& p_value) -> range_iterator_t<range_t>
	{
		return ranges::search_n(std::begin(p_range), std::end(p_range), p_count, p_value);
	}

	template <typename forward_iter, typename size_t, typename type_t, typename binary_pred_t>
	inline auto search_n(forward_iter p_first, forward_iter p_last, size_t p_count, const type_t& p_value, binary_pred_t p_pred) -> forward_iter
	{
		return std::search_n(p_first, p_last, p_count, p_value, p_pred);
	}

	template <typename range_t,
			  typename size_t,
			  typename type_t,
			  typename binary_pred_t,
			  typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto search_n(range_t&& p_range, size_t p_count, const type_t& p_value, binary_pred_t p_pred) -> range_iterator_t<range_t>
	{
		return ranges::search_n(std::begin(p_range), std::end(p_range), p_count, p_value, p_pred);
	}

	template <typename input_iter, typename type_t> inline auto contains(input_iter p_first, input_iter p_last, const type_t& p_value) -> bool
	{
		return ranges::find(p_first, p_last, p_value) != p_last;
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto contains(range_t&& p_range, const type_t& p_value) -> bool
	{
		return ranges::contains(std::begin(p_range), std::end(p_range), p_value);
	}

	template <typename input_iter1, typename input_iter2>
	inline auto starts_with(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2) -> bool
	{
		for (; p_first1 != p_last1 && p_first2 != p_last2; ++p_first1, ++p_first2)
			if (!(*p_first1 == *p_first2))
				return false;
		return p_first2 == p_last2;
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto starts_with(range1_t&& p_range1, range2_t&& p_range2) -> bool
	{
		return ranges::starts_with(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2));
	}

	template <typename input_iter1, typename input_iter2>
	inline auto ends_with(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2) -> bool
	{
		auto len1 = std::distance(p_first1, p_last1);
		auto len2 = std::distance(p_first2, p_last2);
		if (len2 > len1)
			return false;
		return ranges::equal(p_first1 + (len1 - len2), p_last1, p_first2);
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto ends_with(range1_t&& p_range1, range2_t&& p_range2) -> bool
	{
		return ranges::ends_with(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2));
	}

	template <typename input_iter, typename size_t, typename output_iter>
	inline auto copy_n(input_iter p_first, size_t p_n, output_iter p_result) -> in_out_result<input_iter, output_iter>
	{
		for (size_t idx_for = 0; idx_for < p_n; ++idx_for, ++p_first, ++p_result)
			*p_result = *p_first;
		return {p_first, p_result};
	}

	template <typename bidirectional_iter1, typename bidirectional_iter2>
	inline auto copy_backward(bidirectional_iter1 p_first, bidirectional_iter1 p_last, bidirectional_iter2 p_result) -> bidirectional_iter2
	{
		while (p_first != p_last)
			*(--p_result) = *(--p_last);
		return p_result;
	}

	template <typename range_t, typename output_iter, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto copy_backward(range_t&& p_range, output_iter p_result) -> output_iter
	{
		return ranges::copy_backward(std::begin(p_range), std::end(p_range), p_result);
	}

	template <typename input_iter, typename output_iter> inline auto move(input_iter p_first, input_iter p_last, output_iter p_result) -> output_iter
	{
		for (; p_first != p_last; ++p_first, ++p_result)
			*p_result = std::move(*p_first);
		return p_result;
	}

	template <typename range_t, typename output_iter, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto move(range_t&& p_range, output_iter p_result) -> output_iter
	{
		return ranges::move(std::begin(p_range), std::end(p_range), p_result);
	}

	template <typename bidirectional_iter1, typename bidirectional_iter2>
	inline auto move_backward(bidirectional_iter1 p_first, bidirectional_iter1 p_last, bidirectional_iter2 p_result) -> bidirectional_iter2
	{
		while (p_first != p_last)
			*(--p_result) = std::move(*(--p_last));
		return p_result;
	}

	template <typename range_t, typename output_iter, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto move_backward(range_t&& p_range, output_iter p_result) -> output_iter
	{
		return ranges::move_backward(std::begin(p_range), std::end(p_range), p_result);
	}

	template <typename output_iter, typename size_t, typename type_t>
	inline auto fill_n(output_iter p_first, size_t p_n, const type_t& p_value) -> output_iter
	{
		for (size_t idx_for = 0; idx_for < p_n; ++idx_for, ++p_first)
			*p_first = p_value;
		return p_first;
	}

	template <typename output_iter, typename size_t, typename generator_t>
	inline auto generate_n(output_iter p_first, size_t p_n, generator_t p_gen) -> output_iter
	{
		for (size_t idx_for = 0; idx_for < p_n; ++idx_for, ++p_first)
			*p_first = p_gen();
		return p_first;
	}

	template <typename forward_iter1, typename forward_iter2>
	inline auto swap_ranges(forward_iter1 p_first1, forward_iter1 p_last1, forward_iter2 p_first2) -> forward_iter2
	{
		for (; p_first1 != p_last1; ++p_first1, ++p_first2)
			std::iter_swap(p_first1, p_first2);
		return p_first2;
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto swap_ranges(range1_t&& p_range1, range2_t&& p_range2) -> range_iterator_t<range2_t>
	{
		return ranges::swap_ranges(std::begin(p_range1), std::end(p_range1), std::begin(p_range2));
	}

	template <typename forward_iter, typename type_t>
	inline auto replace(forward_iter p_first, forward_iter p_last, const type_t& p_old_value, const type_t& p_new_value) -> void
	{
		for (; p_first != p_last; ++p_first)
			if (*p_first == p_old_value)
				*p_first = p_new_value;
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto replace(range_t&& p_range, const type_t& p_old_value, const type_t& p_new_value) -> void
	{
		ranges::replace(std::begin(p_range), std::end(p_range), p_old_value, p_new_value);
	}

	template <typename forward_iter, typename pred_t, typename type_t>
	inline auto replace_if(forward_iter p_first, forward_iter p_last, pred_t p_pred, const type_t& p_new_value) -> void
	{
		for (; p_first != p_last; ++p_first)
			if (p_pred(*p_first))
				*p_first = p_new_value;
	}

	template <typename range_t, typename pred_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto replace_if(range_t&& p_range, pred_t p_pred, const type_t& p_new_value) -> void
	{
		ranges::replace_if(std::begin(p_range), std::end(p_range), p_pred, p_new_value);
	}

	template <typename forward_iter> inline auto rotate(forward_iter p_first, forward_iter p_middle, forward_iter p_last) -> forward_iter
	{
		return std::rotate(p_first, p_middle, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto rotate(range_t&& p_range, range_iterator_t<range_t> p_middle) -> range_iterator_t<range_t>
	{
		return ranges::rotate(std::begin(p_range), p_middle, std::end(p_range));
	}

	template <typename input_iter, typename output_iter, typename type_t>
	inline auto remove_copy(input_iter p_first, input_iter p_last, output_iter p_result, const type_t& p_value) -> output_iter
	{
		for (; p_first != p_last; ++p_first)
			if (!(*p_first == p_value))
				*p_result++ = std::move(*p_first);
		return p_result;
	}

	template <typename range_t, typename output_iter, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto remove_copy(range_t&& p_range, output_iter p_result, const type_t& p_value) -> output_iter
	{
		return ranges::remove_copy(std::begin(p_range), std::end(p_range), p_result, p_value);
	}

	template <typename input_iter, typename output_iter, typename pred_t>
	inline auto remove_copy_if(input_iter p_first, input_iter p_last, output_iter p_result, pred_t p_pred) -> output_iter
	{
		for (; p_first != p_last; ++p_first)
			if (!p_pred(*p_first))
				*p_result++ = std::move(*p_first);
		return p_result;
	}

	template <typename range_t, typename output_iter, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto remove_copy_if(range_t&& p_range, output_iter p_result, pred_t p_pred) -> output_iter
	{
		return ranges::remove_copy_if(std::begin(p_range), std::end(p_range), p_result, p_pred);
	}

	template <typename forward_iter> inline auto unique(forward_iter p_first, forward_iter p_last) -> forward_iter
	{
		if (p_first == p_last)
			return p_last;
		forward_iter result = p_first;
		++p_first;
		for (; p_first != p_last; ++p_first)
		{
			if (!(*result == *p_first))
			{
				++result;
				if (result != p_first)
					*result = std::move(*p_first);
			}
		}
		return ++result;
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto unique(range_t&& p_range) -> range_iterator_t<range_t>
	{
		return ranges::unique(std::begin(p_range), std::end(p_range));
	}

	template <typename forward_iter, typename binary_pred_t>
	inline auto unique(forward_iter p_first, forward_iter p_last, binary_pred_t p_pred) -> forward_iter
	{
		if (p_first == p_last)
			return p_last;
		forward_iter result = p_first;
		++p_first;
		for (; p_first != p_last; ++p_first)
		{
			if (!p_pred(*result, *p_first))
			{
				++result;
				if (result != p_first)
					*result = std::move(*p_first);
			}
		}
		return ++result;
	}

	template <typename range_t, typename binary_pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto unique(range_t&& p_range, binary_pred_t p_pred) -> range_iterator_t<range_t>
	{
		return ranges::unique(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename input_iter, typename output_iter, typename type_t>
	inline auto replace_copy(input_iter p_first, input_iter p_last, output_iter p_result, const type_t& p_old_value, const type_t& p_new_value)
		-> output_iter
	{
		for (; p_first != p_last; ++p_first, ++p_result)
			*p_result = (*p_first == p_old_value) ? p_new_value : *p_first;
		return p_result;
	}

	template <typename range_t, typename output_iter, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto replace_copy(range_t&& p_range, output_iter p_result, const type_t& p_old_value, const type_t& p_new_value) -> output_iter
	{
		return ranges::replace_copy(std::begin(p_range), std::end(p_range), p_result, p_old_value, p_new_value);
	}

	template <typename input_iter, typename output_iter, typename pred_t, typename type_t>
	inline auto replace_copy_if(input_iter p_first, input_iter p_last, output_iter p_result, pred_t p_pred, const type_t& p_new_value) -> output_iter
	{
		for (; p_first != p_last; ++p_first, ++p_result)
			*p_result = p_pred(*p_first) ? p_new_value : *p_first;
		return p_result;
	}

	template <typename range_t,
			  typename output_iter,
			  typename pred_t,
			  typename type_t,
			  typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto replace_copy_if(range_t&& p_range, output_iter p_result, pred_t p_pred, const type_t& p_new_value) -> output_iter
	{
		return ranges::replace_copy_if(std::begin(p_range), std::end(p_range), p_result, p_pred, p_new_value);
	}

	template <typename bidirectional_iter, typename output_iter>
	inline auto reverse_copy(bidirectional_iter p_first, bidirectional_iter p_last, output_iter p_result) -> output_iter
	{
		while (p_first != p_last)
			*p_result++ = *(--p_last);
		return p_result;
	}

	template <typename range_t, typename output_iter, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto reverse_copy(range_t&& p_range, output_iter p_result) -> output_iter
	{
		return ranges::reverse_copy(std::begin(p_range), std::end(p_range), p_result);
	}

	template <typename forward_iter, typename output_iter>
	inline auto rotate_copy(forward_iter p_first, forward_iter p_middle, forward_iter p_last, output_iter p_result) -> output_iter
	{
		p_result = ranges::copy(p_middle, p_last, p_result);
		return ranges::copy(p_first, p_middle, p_result);
	}

	template <typename range_t, typename output_iter, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto rotate_copy(range_t&& p_range, range_iterator_t<range_t> p_middle, output_iter p_result) -> output_iter
	{
		return ranges::rotate_copy(std::begin(p_range), p_middle, std::end(p_range), p_result);
	}

	template <typename input_iter, typename output_iter>
	inline auto unique_copy(input_iter p_first, input_iter p_last, output_iter p_result) -> output_iter
	{
		if (p_first == p_last)
			return p_result;
		*p_result++ = *p_first++;
		for (; p_first != p_last; ++p_first)
			if (!(*(p_first - 1) == *p_first))
				*p_result++ = std::move(*p_first);
		return p_result;
	}

	template <typename range_t, typename output_iter, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto unique_copy(range_t&& p_range, output_iter p_result) -> output_iter
	{
		return ranges::unique_copy(std::begin(p_range), std::end(p_range), p_result);
	}

	template <typename input_iter, typename output_iter, typename binary_pred_t>
	inline auto unique_copy(input_iter p_first, input_iter p_last, output_iter p_result, binary_pred_t p_pred) -> output_iter
	{
		if (p_first == p_last)
			return p_result;
		*p_result++ = *p_first++;
		for (; p_first != p_last; ++p_first)
			if (!p_pred(*(p_first - 1), *p_first))
				*p_result++ = std::move(*p_first);
		return p_result;
	}

	template <typename range_t, typename output_iter, typename binary_pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto unique_copy(range_t&& p_range, output_iter p_result, binary_pred_t p_pred) -> output_iter
	{
		return ranges::unique_copy(std::begin(p_range), std::end(p_range), p_result, p_pred);
	}

	template <typename input_iter, typename pred_t> inline auto is_partitioned(input_iter p_first, input_iter p_last, pred_t p_pred) -> bool
	{
		for (; p_first != p_last && p_pred(*p_first); ++p_first)
			;
		for (; p_first != p_last; ++p_first)
			if (p_pred(*p_first))
				return false;
		return true;
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto is_partitioned(range_t&& p_range, pred_t p_pred) -> bool
	{
		return ranges::is_partitioned(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename forward_iter, typename pred_t>
	inline auto partition_point(forward_iter p_first, forward_iter p_last, pred_t p_pred) -> forward_iter
	{
		auto len = std::distance(p_first, p_last);
		while (len > 0)
		{
			auto half	= len / 2;
			auto middle = p_first;
			std::advance(middle, half);
			if (p_pred(*middle))
			{
				p_first = ++middle;
				len -= half + 1;
			}
			else
				len = half;
		}
		return p_first;
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto partition_point(range_t&& p_range, pred_t p_pred) -> range_iterator_t<range_t>
	{
		return ranges::partition_point(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename forward_iter, typename pred_t> inline auto partition(forward_iter p_first, forward_iter p_last, pred_t p_pred) -> forward_iter
	{
		return std::partition(p_first, p_last, p_pred);
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto partition(range_t&& p_range, pred_t p_pred) -> range_iterator_t<range_t>
	{
		return ranges::partition(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename bidirectional_iter, typename pred_t>
	inline auto stable_partition(bidirectional_iter p_first, bidirectional_iter p_last, pred_t p_pred) -> bidirectional_iter
	{
		return std::stable_partition(p_first, p_last, p_pred);
	}

	template <typename range_t, typename pred_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto stable_partition(range_t&& p_range, pred_t p_pred) -> range_iterator_t<range_t>
	{
		return ranges::stable_partition(std::begin(p_range), std::end(p_range), p_pred);
	}

	template <typename input_iter, typename output_iter1, typename output_iter2, typename pred_t>
	inline auto partition_copy(input_iter p_first, input_iter p_last, output_iter1 p_out_true, output_iter2 p_out_false, pred_t p_pred)
		-> in_in_result<output_iter1, output_iter2>
	{
		for (; p_first != p_last; ++p_first)
			if (p_pred(*p_first))
				*p_out_true++ = *p_first;
			else
				*p_out_false++ = *p_first;
		return {p_out_true, p_out_false};
	}

	template <typename range_t,
			  typename output_iter1,
			  typename output_iter2,
			  typename pred_t,
			  typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto partition_copy(range_t&& p_range, output_iter1 p_out_true, output_iter2 p_out_false, pred_t p_pred)
		-> in_in_result<output_iter1, output_iter2>
	{
		return ranges::partition_copy(std::begin(p_range), std::end(p_range), p_out_true, p_out_false, p_pred);
	}

	template <typename forward_iter> inline auto is_sorted(forward_iter p_first, forward_iter p_last) -> bool
	{
		return std::is_sorted(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type> inline auto is_sorted(range_t&& p_range) -> bool
	{
		return ranges::is_sorted(std::begin(p_range), std::end(p_range));
	}

	template <typename forward_iter, typename compare_t> inline auto is_sorted(forward_iter p_first, forward_iter p_last, compare_t p_comp) -> bool
	{
		return std::is_sorted(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto is_sorted(range_t&& p_range, compare_t p_comp) -> bool
	{
		return ranges::is_sorted(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename forward_iter> inline auto is_sorted_until(forward_iter p_first, forward_iter p_last) -> forward_iter
	{
		return std::is_sorted_until(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto is_sorted_until(range_t&& p_range) -> range_iterator_t<range_t>
	{
		return ranges::is_sorted_until(std::begin(p_range), std::end(p_range));
	}

	template <typename forward_iter, typename compare_t>
	inline auto is_sorted_until(forward_iter p_first, forward_iter p_last, compare_t p_comp) -> forward_iter
	{
		return std::is_sorted_until(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto is_sorted_until(range_t&& p_range, compare_t p_comp) -> range_iterator_t<range_t>
	{
		return ranges::is_sorted_until(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename random_iter> inline auto stable_sort(random_iter p_first, random_iter p_last) -> void
	{
		std::stable_sort(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type> inline auto stable_sort(range_t&& p_range) -> void
	{
		ranges::stable_sort(std::begin(p_range), std::end(p_range));
	}

	template <typename random_iter, typename compare_t> inline auto stable_sort(random_iter p_first, random_iter p_last, compare_t p_comp) -> void
	{
		std::stable_sort(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto stable_sort(range_t&& p_range, compare_t p_comp) -> void
	{
		ranges::stable_sort(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename random_iter> inline auto partial_sort(random_iter p_first, random_iter p_middle, random_iter p_last) -> void
	{
		std::partial_sort(p_first, p_middle, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto partial_sort(range_t&& p_range, range_iterator_t<range_t> p_middle) -> void
	{
		ranges::partial_sort(std::begin(p_range), p_middle, std::end(p_range));
	}

	template <typename random_iter, typename compare_t>
	inline auto partial_sort(random_iter p_first, random_iter p_middle, random_iter p_last, compare_t p_comp) -> void
	{
		std::partial_sort(p_first, p_middle, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto partial_sort(range_t&& p_range, range_iterator_t<range_t> p_middle, compare_t p_comp) -> void
	{
		ranges::partial_sort(std::begin(p_range), p_middle, std::end(p_range), p_comp);
	}

	template <typename random_iter> inline auto nth_element(random_iter p_first, random_iter p_nth, random_iter p_last) -> void
	{
		std::nth_element(p_first, p_nth, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto nth_element(range_t&& p_range, range_iterator_t<range_t> p_nth) -> void
	{
		ranges::nth_element(std::begin(p_range), p_nth, std::end(p_range));
	}

	template <typename random_iter, typename compare_t>
	inline auto nth_element(random_iter p_first, random_iter p_nth, random_iter p_last, compare_t p_comp) -> void
	{
		std::nth_element(p_first, p_nth, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto nth_element(range_t&& p_range, range_iterator_t<range_t> p_nth, compare_t p_comp) -> void
	{
		ranges::nth_element(std::begin(p_range), p_nth, std::end(p_range), p_comp);
	}

	template <typename input_iter, typename random_iter>
	inline auto partial_sort_copy(input_iter p_first, input_iter p_last, random_iter p_result_first, random_iter p_result_last) -> random_iter
	{
		return std::partial_sort_copy(p_first, p_last, p_result_first, p_result_last);
	}

	template <typename range_t,
			  typename output_range_t,
			  typename = typename std::enable_if<is_range<range_t>::value && is_range<output_range_t>::value>::type>
	inline auto partial_sort_copy(range_t&& p_range, output_range_t&& p_result_range) -> range_iterator_t<output_range_t>
	{
		return ranges::partial_sort_copy(std::begin(p_range), std::end(p_range), std::begin(p_result_range), std::end(p_result_range));
	}

	template <typename input_iter, typename random_iter, typename compare_t>
	inline auto partial_sort_copy(input_iter p_first, input_iter p_last, random_iter p_result_first, random_iter p_result_last, compare_t p_comp)
		-> random_iter
	{
		return std::partial_sort_copy(p_first, p_last, p_result_first, p_result_last, p_comp);
	}

	template <typename range_t,
			  typename output_range_t,
			  typename compare_t,
			  typename = typename std::enable_if<is_range<range_t>::value && is_range<output_range_t>::value>::type>
	inline auto partial_sort_copy(range_t&& p_range, output_range_t&& p_result_range, compare_t p_comp) -> range_iterator_t<output_range_t>
	{
		return ranges::partial_sort_copy(std::begin(p_range), std::end(p_range), std::begin(p_result_range), std::end(p_result_range), p_comp);
	}

	template <typename forward_iter, typename type_t>
	inline auto lower_bound(forward_iter p_first, forward_iter p_last, const type_t& p_value) -> forward_iter
	{
		return std::lower_bound(p_first, p_last, p_value);
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto lower_bound(range_t&& p_range, const type_t& p_value) -> range_iterator_t<range_t>
	{
		return ranges::lower_bound(std::begin(p_range), std::end(p_range), p_value);
	}

	template <typename forward_iter, typename type_t, typename compare_t>
	inline auto lower_bound(forward_iter p_first, forward_iter p_last, const type_t& p_value, compare_t p_comp) -> forward_iter
	{
		return std::lower_bound(p_first, p_last, p_value, p_comp);
	}

	template <typename range_t, typename type_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto lower_bound(range_t&& p_range, const type_t& p_value, compare_t p_comp) -> range_iterator_t<range_t>
	{
		return ranges::lower_bound(std::begin(p_range), std::end(p_range), p_value, p_comp);
	}

	template <typename forward_iter, typename type_t>
	inline auto upper_bound(forward_iter p_first, forward_iter p_last, const type_t& p_value) -> forward_iter
	{
		return std::upper_bound(p_first, p_last, p_value);
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto upper_bound(range_t&& p_range, const type_t& p_value) -> range_iterator_t<range_t>
	{
		return ranges::upper_bound(std::begin(p_range), std::end(p_range), p_value);
	}

	template <typename forward_iter, typename type_t, typename compare_t>
	inline auto upper_bound(forward_iter p_first, forward_iter p_last, const type_t& p_value, compare_t p_comp) -> forward_iter
	{
		return std::upper_bound(p_first, p_last, p_value, p_comp);
	}

	template <typename range_t, typename type_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto upper_bound(range_t&& p_range, const type_t& p_value, compare_t p_comp) -> range_iterator_t<range_t>
	{
		return ranges::upper_bound(std::begin(p_range), std::end(p_range), p_value, p_comp);
	}

	template <typename forward_iter, typename type_t>
	inline auto binary_search(forward_iter p_first, forward_iter p_last, const type_t& p_value) -> bool
	{
		return std::binary_search(p_first, p_last, p_value);
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto binary_search(range_t&& p_range, const type_t& p_value) -> bool
	{
		return ranges::binary_search(std::begin(p_range), std::end(p_range), p_value);
	}

	template <typename forward_iter, typename type_t, typename compare_t>
	inline auto binary_search(forward_iter p_first, forward_iter p_last, const type_t& p_value, compare_t p_comp) -> bool
	{
		return std::binary_search(p_first, p_last, p_value, p_comp);
	}

	template <typename range_t, typename type_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto binary_search(range_t&& p_range, const type_t& p_value, compare_t p_comp) -> bool
	{
		return ranges::binary_search(std::begin(p_range), std::end(p_range), p_value, p_comp);
	}

	template <typename forward_iter, typename type_t>
	inline auto equal_range(forward_iter p_first, forward_iter p_last, const type_t& p_value) -> subrange<forward_iter>
	{
		auto pair_result = std::equal_range(p_first, p_last, p_value);
		return subrange<forward_iter>(pair_result.first, pair_result.second);
	}

	template <typename range_t, typename type_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto equal_range(range_t&& p_range, const type_t& p_value) -> subrange<range_iterator_t<range_t>>
	{
		return ranges::equal_range(std::begin(p_range), std::end(p_range), p_value);
	}

	template <typename forward_iter, typename type_t, typename compare_t>
	inline auto equal_range(forward_iter p_first, forward_iter p_last, const type_t& p_value, compare_t p_comp) -> subrange<forward_iter>
	{
		auto pair_result = std::equal_range(p_first, p_last, p_value, p_comp);
		return subrange<forward_iter>(pair_result.first, pair_result.second);
	}

	template <typename range_t, typename type_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto equal_range(range_t&& p_range, const type_t& p_value, compare_t p_comp) -> subrange<range_iterator_t<range_t>>
	{
		return ranges::equal_range(std::begin(p_range), std::end(p_range), p_value, p_comp);
	}

	template <typename input_iter1, typename input_iter2, typename output_iter>
	inline auto merge(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, output_iter p_result) -> output_iter
	{
		return std::merge(p_first1, p_last1, p_first2, p_last2, p_result);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename output_iter,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto merge(range1_t&& p_range1, range2_t&& p_range2, output_iter p_result) -> output_iter
	{
		return ranges::merge(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_result);
	}

	template <typename input_iter1, typename input_iter2, typename output_iter, typename compare_t>
	inline auto merge(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, output_iter p_result, compare_t p_comp)
		-> output_iter
	{
		return std::merge(p_first1, p_last1, p_first2, p_last2, p_result, p_comp);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename output_iter,
			  typename compare_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto merge(range1_t&& p_range1, range2_t&& p_range2, output_iter p_result, compare_t p_comp) -> output_iter
	{
		return ranges::merge(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_result, p_comp);
	}

	template <typename input_iter1, typename input_iter2, typename output_iter>
	inline auto set_difference(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, output_iter p_result)
		-> output_iter
	{
		return std::set_difference(p_first1, p_last1, p_first2, p_last2, p_result);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename output_iter,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto set_difference(range1_t&& p_range1, range2_t&& p_range2, output_iter p_result) -> output_iter
	{
		return ranges::set_difference(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_result);
	}

	template <typename input_iter1, typename input_iter2, typename output_iter, typename compare_t>
	inline auto
	set_difference(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, output_iter p_result, compare_t p_comp)
		-> output_iter
	{
		return std::set_difference(p_first1, p_last1, p_first2, p_last2, p_result, p_comp);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename output_iter,
			  typename compare_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto set_difference(range1_t&& p_range1, range2_t&& p_range2, output_iter p_result, compare_t p_comp) -> output_iter
	{
		return ranges::set_difference(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_result, p_comp);
	}

	template <typename input_iter1, typename input_iter2, typename output_iter>
	inline auto set_intersection(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, output_iter p_result)
		-> output_iter
	{
		return std::set_intersection(p_first1, p_last1, p_first2, p_last2, p_result);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename output_iter,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto set_intersection(range1_t&& p_range1, range2_t&& p_range2, output_iter p_result) -> output_iter
	{
		return ranges::set_intersection(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_result);
	}

	template <typename input_iter1, typename input_iter2, typename output_iter, typename compare_t>
	inline auto
	set_intersection(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, output_iter p_result, compare_t p_comp)
		-> output_iter
	{
		return std::set_intersection(p_first1, p_last1, p_first2, p_last2, p_result, p_comp);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename output_iter,
			  typename compare_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto set_intersection(range1_t&& p_range1, range2_t&& p_range2, output_iter p_result, compare_t p_comp) -> output_iter
	{
		return ranges::set_intersection(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_result, p_comp);
	}

	template <typename input_iter1, typename input_iter2, typename output_iter>
	inline auto set_union(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, output_iter p_result) -> output_iter
	{
		return std::set_union(p_first1, p_last1, p_first2, p_last2, p_result);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename output_iter,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto set_union(range1_t&& p_range1, range2_t&& p_range2, output_iter p_result) -> output_iter
	{
		return ranges::set_union(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_result);
	}

	template <typename input_iter1, typename input_iter2, typename output_iter, typename compare_t>
	inline auto
	set_union(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, output_iter p_result, compare_t p_comp)
		-> output_iter
	{
		return std::set_union(p_first1, p_last1, p_first2, p_last2, p_result, p_comp);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename output_iter,
			  typename compare_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto set_union(range1_t&& p_range1, range2_t&& p_range2, output_iter p_result, compare_t p_comp) -> output_iter
	{
		return ranges::set_union(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_result, p_comp);
	}

	template <typename input_iter1, typename input_iter2>
	inline auto includes(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2) -> bool
	{
		return std::includes(p_first1, p_last1, p_first2, p_last2);
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto includes(range1_t&& p_range1, range2_t&& p_range2) -> bool
	{
		return ranges::includes(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2));
	}

	template <typename input_iter1, typename input_iter2, typename compare_t>
	inline auto includes(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, compare_t p_comp) -> bool
	{
		return std::includes(p_first1, p_last1, p_first2, p_last2, p_comp);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename compare_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto includes(range1_t&& p_range1, range2_t&& p_range2, compare_t p_comp) -> bool
	{
		return ranges::includes(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_comp);
	}

	template <typename input_iter1, typename input_iter2, typename output_iter>
	inline auto set_symmetric_difference(input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, output_iter p_result)
		-> output_iter
	{
		return std::set_symmetric_difference(p_first1, p_last1, p_first2, p_last2, p_result);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename output_iter,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto set_symmetric_difference(range1_t&& p_range1, range2_t&& p_range2, output_iter p_result) -> output_iter
	{
		return ranges::set_symmetric_difference(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_result);
	}

	template <typename input_iter1, typename input_iter2, typename output_iter, typename compare_t>
	inline auto set_symmetric_difference(
		input_iter1 p_first1, input_iter1 p_last1, input_iter2 p_first2, input_iter2 p_last2, output_iter p_result, compare_t p_comp) -> output_iter
	{
		return std::set_symmetric_difference(p_first1, p_last1, p_first2, p_last2, p_result, p_comp);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename output_iter,
			  typename compare_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto set_symmetric_difference(range1_t&& p_range1, range2_t&& p_range2, output_iter p_result, compare_t p_comp) -> output_iter
	{
		return ranges::set_symmetric_difference(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), std::end(p_range2), p_result, p_comp);
	}

	template <typename bidirectional_iter>
	inline auto inplace_merge(bidirectional_iter p_first, bidirectional_iter p_middle, bidirectional_iter p_last) -> void
	{
		std::inplace_merge(p_first, p_middle, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto inplace_merge(range_t&& p_range, range_iterator_t<range_t> p_middle) -> void
	{
		ranges::inplace_merge(std::begin(p_range), p_middle, std::end(p_range));
	}

	template <typename bidirectional_iter, typename compare_t>
	inline auto inplace_merge(bidirectional_iter p_first, bidirectional_iter p_middle, bidirectional_iter p_last, compare_t p_comp) -> void
	{
		std::inplace_merge(p_first, p_middle, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto inplace_merge(range_t&& p_range, range_iterator_t<range_t> p_middle, compare_t p_comp) -> void
	{
		ranges::inplace_merge(std::begin(p_range), p_middle, std::end(p_range), p_comp);
	}

	template <typename random_iter> inline auto is_heap(random_iter p_first, random_iter p_last) -> bool
	{
		return std::is_heap(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type> inline auto is_heap(range_t&& p_range) -> bool
	{
		return ranges::is_heap(std::begin(p_range), std::end(p_range));
	}

	template <typename random_iter, typename compare_t> inline auto is_heap(random_iter p_first, random_iter p_last, compare_t p_comp) -> bool
	{
		return std::is_heap(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto is_heap(range_t&& p_range, compare_t p_comp) -> bool
	{
		return ranges::is_heap(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename random_iter> inline auto is_heap_until(random_iter p_first, random_iter p_last) -> random_iter
	{
		return std::is_heap_until(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto is_heap_until(range_t&& p_range) -> range_iterator_t<range_t>
	{
		return ranges::is_heap_until(std::begin(p_range), std::end(p_range));
	}

	template <typename random_iter, typename compare_t>
	inline auto is_heap_until(random_iter p_first, random_iter p_last, compare_t p_comp) -> random_iter
	{
		return std::is_heap_until(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto is_heap_until(range_t&& p_range, compare_t p_comp) -> range_iterator_t<range_t>
	{
		return ranges::is_heap_until(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename random_iter> inline auto make_heap(random_iter p_first, random_iter p_last) -> void
	{
		std::make_heap(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type> inline auto make_heap(range_t&& p_range) -> void
	{
		ranges::make_heap(std::begin(p_range), std::end(p_range));
	}

	template <typename random_iter, typename compare_t> inline auto make_heap(random_iter p_first, random_iter p_last, compare_t p_comp) -> void
	{
		std::make_heap(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto make_heap(range_t&& p_range, compare_t p_comp) -> void
	{
		ranges::make_heap(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename random_iter> inline auto sort_heap(random_iter p_first, random_iter p_last) -> void
	{
		std::sort_heap(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type> inline auto sort_heap(range_t&& p_range) -> void
	{
		ranges::sort_heap(std::begin(p_range), std::end(p_range));
	}

	template <typename random_iter, typename compare_t> inline auto sort_heap(random_iter p_first, random_iter p_last, compare_t p_comp) -> void
	{
		std::sort_heap(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto sort_heap(range_t&& p_range, compare_t p_comp) -> void
	{
		ranges::sort_heap(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename random_iter> inline auto push_heap(random_iter p_first, random_iter p_last) -> void
	{
		std::push_heap(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type> inline auto push_heap(range_t&& p_range) -> void
	{
		ranges::push_heap(std::begin(p_range), std::end(p_range));
	}

	template <typename random_iter, typename compare_t> inline auto push_heap(random_iter p_first, random_iter p_last, compare_t p_comp) -> void
	{
		std::push_heap(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto push_heap(range_t&& p_range, compare_t p_comp) -> void
	{
		ranges::push_heap(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename random_iter> inline auto pop_heap(random_iter p_first, random_iter p_last) -> void
	{
		std::pop_heap(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type> inline auto pop_heap(range_t&& p_range) -> void
	{
		ranges::pop_heap(std::begin(p_range), std::end(p_range));
	}

	template <typename random_iter, typename compare_t> inline auto pop_heap(random_iter p_first, random_iter p_last, compare_t p_comp) -> void
	{
		std::pop_heap(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto pop_heap(range_t&& p_range, compare_t p_comp) -> void
	{
		ranges::pop_heap(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename type_t> inline auto max(const type_t& p_a, const type_t& p_b) -> const type_t&
	{
		return (p_a < p_b) ? p_b : p_a;
	}

	template <typename type_t, typename compare_t> inline auto max(const type_t& p_a, const type_t& p_b, compare_t p_comp) -> const type_t&
	{
		return p_comp(p_a, p_b) ? p_b : p_a;
	}

	template <typename type_t> inline auto min(const type_t& p_a, const type_t& p_b) -> const type_t&
	{
		return (p_b < p_a) ? p_b : p_a;
	}

	template <typename type_t, typename compare_t> inline auto min(const type_t& p_a, const type_t& p_b, compare_t p_comp) -> const type_t&
	{
		return p_comp(p_b, p_a) ? p_b : p_a;
	}

	template <typename type_t> inline auto minmax(const type_t& p_a, const type_t& p_b) -> min_max_result<const type_t&, const type_t&>
	{
		return (p_b < p_a) ? min_max_result<const type_t&, const type_t&>{p_b, p_a} : min_max_result<const type_t&, const type_t&>{p_a, p_b};
	}

	template <typename type_t, typename compare_t>
	inline auto minmax(const type_t& p_a, const type_t& p_b, compare_t p_comp) -> min_max_result<const type_t&, const type_t&>
	{
		return p_comp(p_b, p_a) ? min_max_result<const type_t&, const type_t&>{p_b, p_a} : min_max_result<const type_t&, const type_t&>{p_a, p_b};
	}

	template <typename forward_iter>
	inline auto minmax_element(forward_iter p_first, forward_iter p_last) -> min_max_result<forward_iter, forward_iter>
	{
		auto pair_result = std::minmax_element(p_first, p_last);
		return {pair_result.first, pair_result.second};
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto minmax_element(range_t&& p_range) -> min_max_result<range_iterator_t<range_t>, range_iterator_t<range_t>>
	{
		return ranges::minmax_element(std::begin(p_range), std::end(p_range));
	}

	template <typename forward_iter, typename compare_t>
	inline auto minmax_element(forward_iter p_first, forward_iter p_last, compare_t p_comp) -> min_max_result<forward_iter, forward_iter>
	{
		auto pair_result = std::minmax_element(p_first, p_last, p_comp);
		return {pair_result.first, pair_result.second};
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto minmax_element(range_t&& p_range, compare_t p_comp) -> min_max_result<range_iterator_t<range_t>, range_iterator_t<range_t>>
	{
		return ranges::minmax_element(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename type_t> inline auto clamp(const type_t& p_v, const type_t& p_lo, const type_t& p_hi) -> const type_t&
	{
		return (p_v < p_lo) ? p_lo : (p_hi < p_v) ? p_hi : p_v;
	}

	template <typename type_t, typename compare_t>
	inline auto clamp(const type_t& p_v, const type_t& p_lo, const type_t& p_hi, compare_t p_comp) -> const type_t&
	{
		return p_comp(p_v, p_lo) ? p_lo : p_comp(p_hi, p_v) ? p_hi : p_v;
	}

	template <typename forward_iter1, typename forward_iter2>
	inline auto is_permutation(forward_iter1 p_first1, forward_iter1 p_last1, forward_iter2 p_first2) -> bool
	{
		return std::is_permutation(p_first1, p_last1, p_first2);
	}

	template <typename range1_t, typename range2_t, typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto is_permutation(range1_t&& p_range1, range2_t&& p_range2) -> bool
	{
		return ranges::is_permutation(std::begin(p_range1), std::end(p_range1), std::begin(p_range2));
	}

	template <typename forward_iter1, typename forward_iter2, typename binary_pred_t>
	inline auto is_permutation(forward_iter1 p_first1, forward_iter1 p_last1, forward_iter2 p_first2, binary_pred_t p_pred) -> bool
	{
		return std::is_permutation(p_first1, p_last1, p_first2, p_pred);
	}

	template <typename range1_t,
			  typename range2_t,
			  typename binary_pred_t,
			  typename = typename std::enable_if<is_range<range1_t>::value && is_range<range2_t>::value>::type>
	inline auto is_permutation(range1_t&& p_range1, range2_t&& p_range2, binary_pred_t p_pred) -> bool
	{
		return ranges::is_permutation(std::begin(p_range1), std::end(p_range1), std::begin(p_range2), p_pred);
	}

	template <typename bidirectional_iter> inline auto next_permutation(bidirectional_iter p_first, bidirectional_iter p_last) -> bool
	{
		return std::next_permutation(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto next_permutation(range_t&& p_range) -> bool
	{
		return ranges::next_permutation(std::begin(p_range), std::end(p_range));
	}

	template <typename bidirectional_iter, typename compare_t>
	inline auto next_permutation(bidirectional_iter p_first, bidirectional_iter p_last, compare_t p_comp) -> bool
	{
		return std::next_permutation(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto next_permutation(range_t&& p_range, compare_t p_comp) -> bool
	{
		return ranges::next_permutation(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename bidirectional_iter> inline auto prev_permutation(bidirectional_iter p_first, bidirectional_iter p_last) -> bool
	{
		return std::prev_permutation(p_first, p_last);
	}

	template <typename range_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto prev_permutation(range_t&& p_range) -> bool
	{
		return ranges::prev_permutation(std::begin(p_range), std::end(p_range));
	}

	template <typename bidirectional_iter, typename compare_t>
	inline auto prev_permutation(bidirectional_iter p_first, bidirectional_iter p_last, compare_t p_comp) -> bool
	{
		return std::prev_permutation(p_first, p_last, p_comp);
	}

	template <typename range_t, typename compare_t, typename = typename std::enable_if<is_range<range_t>::value>::type>
	inline auto prev_permutation(range_t&& p_range, compare_t p_comp) -> bool
	{
		return ranges::prev_permutation(std::begin(p_range), std::end(p_range), p_comp);
	}

	template <typename iter_t, typename sent_t, typename gen_t> auto shuffle(iter_t p_first, sent_t p_last, gen_t&& p_gen) -> void
	{
		std::shuffle(p_first, p_last, std::forward<gen_t>(p_gen));
	}

	template <typename range_t, typename gen_t> auto shuffle(range_t&& p_range, gen_t&& p_gen) -> void
	{
		ranges::shuffle(std::begin(p_range), std::end(p_range), std::forward<gen_t>(p_gen));
	}

	namespace detail
	{
		template <typename input_iter, typename sent_t, typename output_iter, typename distance_t, typename gen_t, typename distr_t>
		auto sample_forward_impl(input_iter p_first, sent_t p_last, output_iter p_out, distance_t p_n, gen_t&& p_gen, distr_t& p_dist)
			-> in_out_result<input_iter, output_iter>
		{
			using param_t		 = typename distr_t::param_type;
			auto remaining		 = std::distance(p_first, p_last);
			distance_t to_select = std::min(p_n, static_cast<distance_t>(remaining));
			for (; p_first != p_last && to_select > 0; ++p_first, --remaining)
			{
				if (remaining > 0 && p_dist(p_gen, param_t(0, remaining - 1)) < to_select)
				{
					*p_out++ = *p_first;
					--to_select;
				}
			}
			return {p_first, p_out};
		}

		template <typename input_iter, typename sent_t, typename output_iter, typename distance_t, typename gen_t, typename distr_t>
		auto sample_input_impl(input_iter p_first, sent_t p_last, output_iter p_out, distance_t p_n, gen_t&& p_gen, distr_t& p_dist)
			-> in_out_result<input_iter, output_iter>
		{
			using param_t				= typename distr_t::param_type;
			distance_t sample_size		= 0;
			output_iter reservoir_begin = p_out;
			for (; p_first != p_last && sample_size != p_n; ++p_first, ++sample_size)
			{
				*p_out++ = *p_first;
			}
			for (distance_t pop_size = sample_size; p_first != p_last; ++p_first, ++pop_size)
			{
				distance_t idx_for_j = p_dist(p_gen, param_t(0, pop_size));
				if (idx_for_j < p_n)
				{
					*(reservoir_begin + idx_for_j) = *p_first;
				}
			}
			return {p_first, p_out};
		}
	}	 // namespace detail

	template <typename input_iter, typename sent_t, typename output_iter, typename distance_t, typename gen_t>
	auto sample(input_iter p_first, sent_t p_last, output_iter p_out, distance_t p_n, gen_t&& p_gen) ->
		typename std::enable_if<is_input_iterator<input_iter>::value, in_out_result<input_iter, output_iter>>::type
	{
		if (p_n <= 0)
		{
			return {p_first, p_out};
		}
		using diff_t  = iter_difference_t<input_iter>;
		using distr_t = std::uniform_int_distribution<diff_t>;
		distr_t dist;
		if (p_n > static_cast<distance_t>(std::numeric_limits<diff_t>::max()))
		{
			return {p_first, p_out};
		}
		diff_t n_samples = static_cast<diff_t>(p_n);
		if (is_forward_iterator<input_iter>::value)
		{
			return detail::sample_forward_impl(p_first, p_last, p_out, n_samples, std::forward<gen_t>(p_gen), dist);
		}
		else
		{
			return detail::sample_input_impl(p_first, p_last, p_out, n_samples, std::forward<gen_t>(p_gen), dist);
		}
	}

	template <typename range_t, typename output_iter, typename distance_t, typename gen_t>
	auto sample(range_t&& p_range, output_iter p_out, distance_t p_n, gen_t&& p_gen) ->
		typename std::enable_if<is_input_range<range_t>::value, in_out_result<borrowed_iterator_t<range_t>, output_iter>>::type
	{
		auto result = ranges::sample(std::begin(p_range), std::end(p_range), p_out, p_n, std::forward<gen_t>(p_gen));
		return {borrowed_iterator_t<range_t>(result.in), result.out};
	}

	template <typename container_t> inline auto to() -> views::to_partial<container_t>
	{
		return views::to_partial<container_t>{};
	}
}	 // namespace ranges

template <typename range_t, typename func_t>
inline auto operator|(range_t&& p_range, const typename ranges::views::filter_adaptor::partial<func_t>& p_adaptor)
	-> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t, typename func_t>
inline auto operator|(const range_t& p_range, const typename ranges::views::filter_adaptor::partial<func_t>& p_adaptor)
	-> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t, typename func_t>
inline auto operator|(range_t&& p_range, const typename ranges::views::transform_adaptor::partial<func_t>& p_adaptor)
	-> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t, typename func_t>
inline auto operator|(const range_t& p_range, const typename ranges::views::transform_adaptor::partial<func_t>& p_adaptor)
	-> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::take_adaptor::partial& p_adaptor) -> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::take_adaptor::partial& p_adaptor) -> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::drop_adaptor::partial& p_adaptor) -> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::drop_adaptor::partial& p_adaptor) -> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::sort_adaptor::partial& p_adaptor) -> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::sort_adaptor::partial& p_adaptor) -> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::sort_adaptor& p_adaptor) -> decltype(p_adaptor()(std::forward<range_t>(p_range)))
{
	return p_adaptor()(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::sort_adaptor& p_adaptor) -> decltype(p_adaptor()(p_range))
{
	return p_adaptor()(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::unique_adaptor::partial& p_adaptor)
	-> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::unique_adaptor::partial& p_adaptor) -> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::unique_adaptor& p_adaptor) -> decltype(p_adaptor()(std::forward<range_t>(p_range)))
{
	return p_adaptor()(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::unique_adaptor& p_adaptor) -> decltype(p_adaptor()(p_range))
{
	return p_adaptor()(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::enumerate_adaptor::partial& p_adaptor)
	-> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::enumerate_adaptor::partial& p_adaptor) -> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::enumerate_adaptor& p_adaptor) -> decltype(p_adaptor()(std::forward<range_t>(p_range)))
{
	return p_adaptor()(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::enumerate_adaptor& p_adaptor) -> decltype(p_adaptor()(p_range))
{
	return p_adaptor()(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::join_adaptor::partial& p_adaptor) -> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::join_adaptor::partial& p_adaptor) -> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::common_adaptor::partial& p_adaptor)
	-> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::common_adaptor::partial& p_adaptor) -> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t>
inline auto operator|(range_t&& p_range, const ranges::views::reverse_adaptor::partial& p_adaptor)
	-> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t>
inline auto operator|(const range_t& p_range, const ranges::views::reverse_adaptor::partial& p_adaptor) -> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t, typename container_t>
inline auto operator|(range_t&& p_range, const ranges::views::to_partial<container_t>& p_adaptor)
	-> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t, typename container_t>
inline auto operator|(const range_t& p_range, const ranges::views::to_partial<container_t>& p_adaptor) -> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}

template <typename range_t, typename delimiter_t>
inline auto operator|(range_t&& p_range, const ranges::views::join_with_adaptor::partial<delimiter_t>& p_adaptor)
	-> decltype(p_adaptor(std::forward<range_t>(p_range)))
{
	return p_adaptor(std::forward<range_t>(p_range));
}

template <typename range_t, typename delimiter_t>
inline auto operator|(const range_t& p_range, const ranges::views::join_with_adaptor::partial<delimiter_t>& p_adaptor) -> decltype(p_adaptor(p_range))
{
	return p_adaptor(p_range);
}
#endif
