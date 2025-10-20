#ifndef CONCEPTS_HPP
#define CONCEPTS_HPP

#include <type_traits>

namespace utils
{
	template <typename type_t> struct is_default_constructible
	{
		static constexpr bool value = std::is_default_constructible<type_t>::value;
	};

	template <typename type_t> struct is_copy_constructible
	{
		static constexpr bool value = std::is_copy_constructible<type_t>::value;
	};

	template <typename type_t> struct is_move_constructible
	{
		static constexpr bool value = std::is_move_constructible<type_t>::value;
	};

	template <typename type_t> struct is_nothrow_move_constructible
	{
		static constexpr bool value = std::is_nothrow_move_constructible<type_t>::value;
	};

	template <typename type_t> struct is_copy_assignable
	{
		static constexpr bool value = std::is_copy_assignable<type_t>::value;
	};

	template <typename type_t> struct is_move_assignable
	{
		static constexpr bool value = std::is_move_assignable<type_t>::value;
	};

	template <typename type_t> struct is_destructible
	{
		static constexpr bool value = std::is_destructible<type_t>::value;
	};

	template <typename from_t, typename to_t> struct is_convertible
	{
		static constexpr bool value = std::is_convertible<from_t, to_t>::value;
	};

	template <typename to_t, typename from_t> struct is_assignable
	{
		static constexpr bool value = std::is_assignable<to_t, from_t>::value;
	};

	template <typename type1_t, typename type2_t> struct is_same
	{
		static constexpr bool value = false;
	};

	template <typename type_t> struct is_same<type_t, type_t>
	{
		static constexpr bool value = true;
	};

	template <typename type1_t, typename type2_t> struct is_same_v
	{
		static constexpr bool value = is_same<type1_t, type2_t>::value;
	};

	template <bool cond, typename type_t = void> struct enable_if
	{
		using type = type_t;
	};

	template <typename type_t> struct enable_if<false, type_t>
	{
	};

	template <bool cond, typename type_t = void> using enable_if_t = typename enable_if<cond, type_t>::type;

	template <typename...> struct conjunction : std::true_type
	{
	};

	template <typename cond_t> struct conjunction<cond_t> : cond_t
	{
	};

	template <typename cond_t, typename... rest_t>
	struct conjunction<cond_t, rest_t...> : std::conditional<cond_t::value, conjunction<rest_t...>, cond_t>::type
	{
	};

	template <typename...> struct disjunction : std::false_type
	{
	};

	template <typename cond_t> struct disjunction<cond_t> : cond_t
	{
	};

	template <typename cond_t, typename... rest_t>
	struct disjunction<cond_t, rest_t...> : std::conditional<cond_t::value, cond_t, disjunction<rest_t...> >::type
	{
	};

	template <typename cond_t> struct negation : std::integral_constant<bool, !cond_t::value>
	{
	};

	template <typename type_t> using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<type_t>::type>::type;
	template <typename type_t> using decay_t		= typename std::decay<type_t>::type;

	template <typename err_t> struct is_valid_expected_error
	{
		static constexpr bool value = conjunction<is_destructible<err_t>,
												  std::is_object<err_t>,
												  negation<std::is_array<err_t> >,
												  negation<std::is_const<err_t> >,
												  negation<std::is_volatile<err_t> > >::value;
	};
}	 // namespace utils
#endif