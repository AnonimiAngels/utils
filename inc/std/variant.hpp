#pragma once
#ifndef STD_VARIANT_HPP
	#define STD_VARIANT_HPP

	#include "utils_macros.hpp"

	#if MACRO_CXX17_ENABLED
		#include <utility>
		#include <variant>
	#else
		#include <algorithm>
		#include <array>
		#include <cstddef>
		#include <type_traits>
		#include <utility>

namespace std
{
	// Forward declarations
	template <typename... types_t> class variant;

	// Variant npos constant
	constexpr std::size_t variant_npos = static_cast<std::size_t>(-1);

	// Bad variant access exception
	class bad_variant_access : public std::exception
	{
	private:
		const char* m_what;

	public:
		explicit bad_variant_access(const char* p_msg = "bad variant access") : m_what(p_msg) {}

		MACRO_NODISCARD auto what() const noexcept -> const char* override { return m_what; }
	};

	// Variant size
	template <typename variant_t> struct variant_size;

	// Variant alternative
	template <std::size_t index_t, typename variant_t> struct variant_alternative;

	// Monostate
	struct monostate
	{
	};

		#if !MACRO_CXX14_ENABLED
	// Index sequence helpers
	template <std::size_t... indices_t> struct index_sequence
	{
	};

	template <std::size_t n_t, std::size_t... indices_t> struct make_index_sequence_impl : make_index_sequence_impl<n_t - 1, n_t - 1, indices_t...>
	{
	};

	template <std::size_t... indices_t> struct make_index_sequence_impl<0, indices_t...>
	{
		using type = index_sequence<indices_t...>;
	};

	template <std::size_t n_t> using make_index_sequence = typename make_index_sequence_impl<n_t>::type;
		#endif

	// Variant implementation details
	namespace detail
	{
		// Check if types contain a specific type
		template <typename target_t, typename... types_t> struct contains : std::false_type
		{
		};

		template <typename target_t, typename head_t, typename... tail_t>
		struct contains<target_t, head_t, tail_t...>
			: std::conditional<std::is_same<target_t, head_t>::value, std::true_type, contains<target_t, tail_t...> >::type
		{
		};

		// Get index of type
		template <typename target_t, typename... types_t> struct index_of;

		template <typename target_t> struct index_of<target_t>
		{
			static constexpr std::size_t value = 0;
		};

		template <typename target_t, typename head_t, typename... tail_t> struct index_of<target_t, head_t, tail_t...>
		{
			static constexpr std::size_t value = std::is_same<target_t, head_t>::value ? 0 : 1 + index_of<target_t, tail_t...>::value;
		};

		// Get type at index
		template <std::size_t index_t, typename... types_t> struct at_index;

		template <typename head_t, typename... tail_t> struct at_index<0, head_t, tail_t...>
		{
			using type = head_t;
		};

		template <std::size_t index_t, typename head_t, typename... tail_t> struct at_index<index_t, head_t, tail_t...>
		{
			using type = typename at_index<index_t - 1, tail_t...>::type;
		};

		// Max value computation
		template <std::size_t... values_t> struct max_value;

		template <> struct max_value<>
		{
			static constexpr std::size_t value = 0;
		};

		template <std::size_t value_t> struct max_value<value_t>
		{
			static constexpr std::size_t value = value_t;
		};

		template <std::size_t first_t, std::size_t second_t, std::size_t... rest_t>
		struct max_value<first_t, second_t, rest_t...> : max_value<(first_t > second_t ? first_t : second_t), rest_t...>
		{
		};

		// Aligned storage type
		template <std::size_t len_t, std::size_t align_t = alignof(std::max_align_t)>
		using aligned_storage_t = typename std::aligned_storage<len_t, align_t>::type;

		// Check if type is a variant
		template <typename type_t, typename variant_t> struct is_variant : std::false_type
		{
		};

		template <typename variant_t> struct is_variant<variant_t, variant_t> : std::true_type
		{
		};

		template <typename variant_t> struct is_variant<const variant_t, variant_t> : std::true_type
		{
		};

		template <typename variant_t> struct is_variant<volatile variant_t, variant_t> : std::true_type
		{
		};

		template <typename variant_t> struct is_variant<const volatile variant_t, variant_t> : std::true_type
		{
		};

		// Destructor table
		template <typename... types_t> struct destructor
		{
			template <typename type_t> static auto destroy_impl(void* p_storage) -> void { reinterpret_cast<type_t*>(p_storage)->~type_t(); }

			template <std::size_t... indices_t>
			static auto make_table_impl(index_sequence<indices_t...>) -> std::array<void (*)(void*), sizeof...(indices_t)>
			{
				return {{&destroy_impl<typename at_index<indices_t, types_t...>::type>...}};
			}

			static auto make_table() -> std::array<void (*)(void*), sizeof...(types_t)>
			{
				return make_table_impl(make_index_sequence<sizeof...(types_t)>{});
			}
		};

		// Copy constructor table
		template <typename... types_t> struct copy_constructor
		{
			template <typename type_t> static auto copy_impl(void* p_dest, const void* p_src) -> void
			{
				::new (p_dest) type_t(*reinterpret_cast<const type_t*>(p_src));
			}

			template <std::size_t... indices_t>
			static auto make_table_impl(index_sequence<indices_t...>) -> std::array<void (*)(void*, const void*), sizeof...(indices_t)>
			{
				return {{&copy_impl<typename at_index<indices_t, types_t...>::type>...}};
			}

			static auto make_table() -> std::array<void (*)(void*, const void*), sizeof...(types_t)>
			{
				return make_table_impl(make_index_sequence<sizeof...(types_t)>{});
			}
		};

		// Move constructor table
		template <typename... types_t> struct move_constructor
		{
			template <typename type_t> static auto move_impl(void* p_dest, void* p_src) -> void
			{
				::new (p_dest) type_t(std::move(*reinterpret_cast<type_t*>(p_src)));
			}

			template <std::size_t... indices_t>
			static auto make_table_impl(index_sequence<indices_t...>) -> std::array<void (*)(void*, void*), sizeof...(indices_t)>
			{
				return {{&move_impl<typename at_index<indices_t, types_t...>::type>...}};
			}

			static auto make_table() -> std::array<void (*)(void*, void*), sizeof...(types_t)>
			{
				return make_table_impl(make_index_sequence<sizeof...(types_t)>{});
			}
		};

		// First type with convertible check
		template <typename type_t, typename variant_t, typename = void> struct first_convertible_index
		{
			static constexpr std::size_t value = variant_npos;
		};

		template <typename type_t, typename... types_t> struct first_convertible_index<type_t, variant<types_t...>, void>
		{
			template <std::size_t idx_t> struct impl
			{
				static constexpr std::size_t value = variant_npos;
			};

			template <std::size_t idx_t> static constexpr auto find() -> std::size_t
			{
				return idx_t < sizeof...(types_t) ? (std::is_constructible<typename at_index<idx_t, types_t...>::type, type_t>::value
														 ? (impl<idx_t>::value != variant_npos ? impl<idx_t>::value : find<idx_t + 1>())
														 : variant_npos)
												  : variant_npos;
			}

			static constexpr std::size_t value = find<0>();
		};
	}	 // namespace detail

	// Main variant class
	template <typename... types_t> class variant
	{
	public:
		// Type aliases
		using self_t = variant;
		using types	 = variant;	   // For visit_invoker access

	private:
		// Member variables
		detail::aligned_storage_t<detail::max_value<sizeof(types_t)...>::value, detail::max_value<alignof(types_t)...>::value> m_storage;
		std::size_t m_index;

	public:
		// Destructor
		~variant() { destroy(); }

		// Constructors
		variant() : m_index(0)
		{
			using first_t = typename detail::at_index<0, types_t...>::type;
			::new (static_cast<void*>(&m_storage)) first_t{};
		}

		template <typename type_t,
				  typename = typename std::enable_if<!detail::is_variant<type_t, self_t>::value
													 && detail::contains<typename std::decay<type_t>::type, types_t...>::value>::type>
		variant(type_t&& p_value) : m_index(detail::index_of<typename std::decay<type_t>::type, types_t...>::value)
		{
			using target_t = typename std::decay<type_t>::type;
			::new (static_cast<void*>(&m_storage)) target_t(std::forward<type_t>(p_value));
		}

		variant(const self_t& p_other) : m_index(p_other.m_index)
		{
			if (p_other.m_index != variant_npos)
			{
				copy_construct(p_other.m_index, &p_other.m_storage);
			}
		}

		variant(self_t&& p_other) noexcept : m_index(p_other.m_index)
		{
			if (p_other.m_index != variant_npos)
			{
				move_construct(p_other.m_index, &p_other.m_storage);
			}
		}

		auto operator=(const self_t& p_other) -> self_t&
		{
			if (this != &p_other)
			{
				destroy();
				m_index = p_other.m_index;
				if (p_other.m_index != variant_npos)
				{
					copy_construct(p_other.m_index, &p_other.m_storage);
				}
			}
			return *this;
		}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				destroy();
				m_index = p_other.m_index;
				if (p_other.m_index != variant_npos)
				{
					move_construct(p_other.m_index, &p_other.m_storage);
				}
			}
			return *this;
		}

	private:
		// Static methods
		static auto get_destructor_table() -> const std::array<void (*)(void*), sizeof...(types_t)>&
		{
			static const auto table = detail::destructor<types_t...>::make_table();
			return table;
		}

		static auto get_copy_constructor_table() -> const std::array<void (*)(void*, const void*), sizeof...(types_t)>&
		{
			static const auto table = detail::copy_constructor<types_t...>::make_table();
			return table;
		}

		static auto get_move_constructor_table() -> const std::array<void (*)(void*, void*), sizeof...(types_t)>&
		{
			static const auto table = detail::move_constructor<types_t...>::make_table();
			return table;
		}

	public:
		// Public methods
		MACRO_NODISCARD auto index() const noexcept -> std::size_t { return m_index; }

		MACRO_NODISCARD auto valueless_by_exception() const noexcept -> bool { return m_index == variant_npos; }

		template <std::size_t idx_t, typename... args_t> auto emplace(args_t&&... p_args) -> typename detail::at_index<idx_t, types_t...>::type&
		{
			static_assert(idx_t < sizeof...(types_t), "index out of bounds");
			destroy();
			m_index		 = idx_t;
			using type_t = typename detail::at_index<idx_t, types_t...>::type;
			::new (static_cast<void*>(&m_storage)) type_t(std::forward<args_t>(p_args)...);
			return *reinterpret_cast<type_t*>(&m_storage);
		}

		template <typename type_t, typename... args_t> auto emplace(args_t&&... p_args) -> type_t&
		{
			static_assert(detail::contains<type_t, types_t...>::value, "type not in variant");
			return emplace<detail::index_of<type_t, types_t...>::value>(std::forward<args_t>(p_args)...);
		}

		auto swap(self_t& p_other) noexcept -> void
		{
			self_t temp(std::move(*this));
			*this	= std::move(p_other);
			p_other = std::move(temp);
		}

		template <typename type_t>
		auto operator=(type_t&& p_value) -> typename std::enable_if<!detail::is_variant<type_t, self_t>::value
																		&& detail::contains<typename std::decay<type_t>::type, types_t...>::value,
																	self_t&>::type
		{
			using target_t			  = typename std::decay<type_t>::type;
			constexpr std::size_t idx = detail::index_of<target_t, types_t...>::value;

			if (m_index == idx)
			{
				*reinterpret_cast<target_t*>(&m_storage) = std::forward<type_t>(p_value);
			}
			else
			{
				emplace<idx>(std::forward<type_t>(p_value));
			}

			return *this;
		}

	private:
		// Private methods
		auto destroy() -> void
		{
			if (m_index != variant_npos)
			{
				get_destructor_table()[m_index](static_cast<void*>(&m_storage));
				m_index = variant_npos;
			}
		}

		auto copy_construct(std::size_t p_index, const void* p_src) -> void
		{
			get_copy_constructor_table()[p_index](static_cast<void*>(&m_storage), p_src);
		}

		auto move_construct(std::size_t p_index, void* p_src) -> void
		{
			get_move_constructor_table()[p_index](static_cast<void*>(&m_storage), p_src);
		}

		// Friend functions for get access
		template <std::size_t idx_t, typename... args_t>
		friend auto get(variant<args_t...>& p_v) -> typename detail::at_index<idx_t, args_t...>::type&;

		template <std::size_t idx_t, typename... args_t>
		friend auto get(const variant<args_t...>& p_v) -> const typename detail::at_index<idx_t, args_t...>::type&;

		template <std::size_t idx_t, typename... args_t>
		friend auto get(variant<args_t...>&& p_v) -> typename detail::at_index<idx_t, args_t...>::type&&;

		template <typename type_t, typename... args_t> friend auto get(variant<args_t...>& p_v) -> type_t&;

		template <typename type_t, typename... args_t> friend auto get(const variant<args_t...>& p_v) -> const type_t&;

		template <typename type_t, typename... args_t> friend auto get(variant<args_t...>&& p_v) -> type_t&&;

		template <std::size_t idx_t, typename... args_t>
		friend auto get_if(variant<args_t...>* p_v) noexcept -> typename detail::at_index<idx_t, args_t...>::type*;

		template <std::size_t idx_t, typename... args_t>
		friend auto get_if(const variant<args_t...>* p_v) noexcept -> const typename detail::at_index<idx_t, args_t...>::type*;

		template <typename type_t, typename... args_t> friend auto get_if(variant<args_t...>* p_v) noexcept -> type_t*;

		template <typename type_t, typename... args_t> friend auto get_if(const variant<args_t...>* p_v) noexcept -> const type_t*;
	};

	// Get by index
	template <std::size_t index_t, typename... types_t> auto get(variant<types_t...>& p_v) -> typename detail::at_index<index_t, types_t...>::type&
	{
		static_assert(index_t < sizeof...(types_t), "index out of bounds");
		if (p_v.m_index != index_t)
		{
			MACRO_THROW(bad_variant_access("variant does not hold alternative at index"));
		}
		using type_t = typename detail::at_index<index_t, types_t...>::type;
		return *reinterpret_cast<type_t*>(&p_v.m_storage);
	}

	template <std::size_t index_t, typename... types_t>
	auto get(const variant<types_t...>& p_v) -> const typename detail::at_index<index_t, types_t...>::type&
	{
		static_assert(index_t < sizeof...(types_t), "index out of bounds");
		if (p_v.m_index != index_t)
		{
			MACRO_THROW(bad_variant_access("variant does not hold alternative at index"));
		}
		using type_t = typename detail::at_index<index_t, types_t...>::type;
		return *reinterpret_cast<const type_t*>(&p_v.m_storage);
	}

	template <std::size_t index_t, typename... types_t> auto get(variant<types_t...>&& p_v) -> typename detail::at_index<index_t, types_t...>::type&&
	{
		static_assert(index_t < sizeof...(types_t), "index out of bounds");
		if (p_v.m_index != index_t)
		{
			MACRO_THROW(bad_variant_access("variant does not hold alternative at index"));
		}
		using type_t = typename detail::at_index<index_t, types_t...>::type;
		return std::move(*reinterpret_cast<type_t*>(&p_v.m_storage));
	}

	// Get by type
	template <typename type_t, typename... types_t> auto get(variant<types_t...>& p_v) -> type_t&
	{
		static_assert(detail::contains<type_t, types_t...>::value, "type not in variant");
		constexpr std::size_t index = detail::index_of<type_t, types_t...>::value;
		return get<index>(p_v);
	}

	template <typename type_t, typename... types_t> auto get(const variant<types_t...>& p_v) -> const type_t&
	{
		static_assert(detail::contains<type_t, types_t...>::value, "type not in variant");
		constexpr std::size_t index = detail::index_of<type_t, types_t...>::value;
		return get<index>(p_v);
	}

	template <typename type_t, typename... types_t> auto get(variant<types_t...>&& p_v) -> type_t&&
	{
		static_assert(detail::contains<type_t, types_t...>::value, "type not in variant");
		constexpr std::size_t index = detail::index_of<type_t, types_t...>::value;
		return get<index>(std::move(p_v));
	}

	// Get if by index
	template <std::size_t index_t, typename... types_t>
	auto get_if(variant<types_t...>* p_v) noexcept -> typename detail::at_index<index_t, types_t...>::type*
	{
		if (p_v && p_v->m_index == index_t)
		{
			using type_t = typename detail::at_index<index_t, types_t...>::type;
			return reinterpret_cast<type_t*>(&p_v->m_storage);
		}
		return nullptr;
	}

	template <std::size_t index_t, typename... types_t>
	auto get_if(const variant<types_t...>* p_v) noexcept -> const typename detail::at_index<index_t, types_t...>::type*
	{
		if (p_v && p_v->m_index == index_t)
		{
			using type_t = typename detail::at_index<index_t, types_t...>::type;
			return reinterpret_cast<const type_t*>(&p_v->m_storage);
		}
		return nullptr;
	}

	// Get if by type
	template <typename type_t, typename... types_t> auto get_if(variant<types_t...>* p_v) noexcept -> type_t*
	{
		static_assert(detail::contains<type_t, types_t...>::value, "type not in variant");
		constexpr std::size_t index = detail::index_of<type_t, types_t...>::value;
		return get_if<index>(p_v);
	}

	template <typename type_t, typename... types_t> auto get_if(const variant<types_t...>* p_v) noexcept -> const type_t*
	{
		static_assert(detail::contains<type_t, types_t...>::value, "type not in variant");
		constexpr std::size_t index = detail::index_of<type_t, types_t...>::value;
		return get_if<index>(p_v);
	}

	// Holds alternative
	template <typename type_t, typename... types_t> auto holds_alternative(const variant<types_t...>& p_v) noexcept -> bool
	{
		static_assert(detail::contains<type_t, types_t...>::value, "type not in variant");
		return p_v.index() == detail::index_of<type_t, types_t...>::value;
	}

	// Variant size
	template <typename variant_t> struct variant_size;

	template <typename... types_t> struct variant_size<variant<types_t...> > : std::integral_constant<std::size_t, sizeof...(types_t)>
	{
	};

	template <typename variant_t> struct variant_size<const variant_t> : variant_size<variant_t>
	{
	};

	template <typename variant_t> struct variant_size<volatile variant_t> : variant_size<variant_t>
	{
	};

	template <typename variant_t> struct variant_size<const volatile variant_t> : variant_size<variant_t>
	{
	};

	// Variant alternative
	template <std::size_t index_t, typename... types_t> struct variant_alternative<index_t, variant<types_t...> >
	{
		static_assert(index_t < sizeof...(types_t), "index out of bounds");
		using type = typename detail::at_index<index_t, types_t...>::type;
	};

	template <std::size_t index_t, typename variant_t> struct variant_alternative<index_t, const variant_t>
	{
		using type = const typename variant_alternative<index_t, variant_t>::type;
	};

	template <std::size_t index_t, typename variant_t> struct variant_alternative<index_t, volatile variant_t>
	{
		using type = volatile typename variant_alternative<index_t, variant_t>::type;
	};

	template <std::size_t index_t, typename variant_t> struct variant_alternative<index_t, const volatile variant_t>
	{
		using type = const volatile typename variant_alternative<index_t, variant_t>::type;
	};

	template <std::size_t index_t, typename variant_t> using variant_alternative_t = typename variant_alternative<index_t, variant_t>::type;

	// Visit implementation
	namespace detail
	{
		template <typename visitor_t, typename variant_t, typename return_t, typename... variant_types_t> struct visit_invoker
		{
			template <std::size_t idx_t> static auto invoke(visitor_t&& p_visitor, variant_t&& p_variant) -> return_t
			{
				return std::forward<visitor_t>(p_visitor)(get<idx_t>(std::forward<variant_t>(p_variant)));
			}

			template <std::size_t... indices_t>
			static auto make_table_impl(index_sequence<indices_t...>) -> std::array<return_t (*)(visitor_t&&, variant_t&&), sizeof...(indices_t)>
			{
				return {{&invoke<indices_t>...}};
			}

			// Fixed: Removed unused parameter p_size
			static auto make_table() -> std::array<return_t (*)(visitor_t&&, variant_t&&), sizeof...(variant_types_t)>
			{
				return make_table_impl(make_index_sequence<sizeof...(variant_types_t)>{});
			}
		};

		template <typename visitor_t, typename variant_t> struct visit_impl
		{
			template <std::size_t index_t = 0>
			static auto apply(visitor_t&& p_visitor, variant_t&& p_variant) ->
				typename std::enable_if<index_t >= variant_size<typename std::decay<variant_t>::type>::value,
										decltype(p_visitor(get<0>(std::forward<variant_t>(p_variant))))>::type
			{
				MACRO_THROW(bad_variant_access("variant in invalid state"));
			}

			template <std::size_t index_t = 0>
				static auto apply(visitor_t&& p_visitor, variant_t&& p_variant) -> typename std::enable_if
				< index_t<variant_size<typename std::decay<variant_t>::type>::value,
						  decltype(p_visitor(get<0>(std::forward<variant_t>(p_variant))))>::type
			{
				if (p_variant.index() == index_t)
				{
					return p_visitor(get<index_t>(std::forward<variant_t>(p_variant)));
				}
				return apply<index_t + 1>(std::forward<visitor_t>(p_visitor), std::forward<variant_t>(p_variant));
			}
		};
	}	 // namespace detail

	// Visit function
	template <typename visitor_t, typename variant_t>
	auto visit(visitor_t&& p_visitor, variant_t&& p_variant) -> decltype(p_visitor(get<0>(std::forward<variant_t>(p_variant))))
	{
		return detail::visit_impl<visitor_t, variant_t>::template apply<0>(std::forward<visitor_t>(p_visitor), std::forward<variant_t>(p_variant));
	}

	// Swap
	template <typename... types_t> auto swap(variant<types_t...>& p_lhs, variant<types_t...>& p_rhs) noexcept -> void
	{
		p_lhs.swap(p_rhs);
	}

	// Comparison operators
	inline auto operator==(monostate /* p_lvar*/, monostate /* p_rvar*/) noexcept -> bool
	{
		return true;
	}

	inline auto operator!=(monostate /* p_lvar*/, monostate /* p_rvar*/) noexcept -> bool
	{
		return false;
	}

	inline auto operator<(monostate /* p_lvar*/, monostate /* p_rvar*/) noexcept -> bool
	{
		return false;
	}

	inline auto operator>(monostate /* p_lvar*/, monostate /* p_rvar*/) noexcept -> bool
	{
		return false;
	}

	inline auto operator<=(monostate /* p_lvar*/, monostate /* p_rvar*/) noexcept -> bool
	{
		return true;
	}

	inline auto operator>=(monostate /* p_lvar*/, monostate /* p_rvar*/) noexcept -> bool
	{
		return true;
	}

}	 // namespace std
	#endif

#endif	  // STD_VARIANT_HPP
