#pragma once
#ifndef CLAUNDER_HPP
	#define CLAUNDER_HPP

	#include <cstdint>
	#include <stdexcept>
	#include <type_traits>

namespace utils
{
	namespace detail
	{
		template <typename type_t> struct is_function_impl : std::false_type
		{
		};

		template <typename ret_t, typename... args_t> struct is_function_impl<ret_t(args_t...)> : std::true_type
		{
		};

		template <typename ret_t, typename... args_t> struct is_function_impl<ret_t(args_t...) const> : std::true_type
		{
		};

		template <typename ret_t, typename... args_t> struct is_function_impl<ret_t(args_t...) volatile> : std::true_type
		{
		};

		template <typename ret_t, typename... args_t> struct is_function_impl<ret_t(args_t...) const volatile> : std::true_type
		{
		};

		template <typename type_t> struct is_function : is_function_impl<typename std::remove_cv<type_t>::type>
		{
		};

		template <typename type_t> struct is_void_pointer : std::false_type
		{
		};

		template <> struct is_void_pointer<void*> : std::true_type
		{
		};

		template <> struct is_void_pointer<const void*> : std::true_type
		{
		};

		template <> struct is_void_pointer<volatile void*> : std::true_type
		{
		};

		template <> struct is_void_pointer<const volatile void*> : std::true_type
		{
		};

		template <typename type_t> struct is_launderable : std::integral_constant<bool, !is_function<type_t>::value && !is_void_pointer<type_t>::value>
		{
		};
	}	 // namespace detail

	template <typename type_t> class claunder
	{
	public:
		using self_t = claunder;

	private:
		claunder()									 = delete;
		~claunder()									 = delete;
		claunder(const self_t&)						 = delete;
		auto operator=(const self_t&) -> self_t&	 = delete;
		claunder(self_t&&) noexcept					 = delete;
		auto operator=(self_t&&) noexcept -> self_t& = delete;

	public:
		template <typename element_t,
				  typename std::enable_if<detail::is_launderable<element_t>::value && std::is_same<typename std::remove_cv<element_t>::type, typename std::remove_cv<type_t>::type>::value,
										  std::int32_t>::type = 0>
		static auto launder(element_t* p_ptr) noexcept -> element_t*
		{
			static_assert(!detail::is_function<element_t>::value, "Cannot launder function pointers");
			static_assert(!detail::is_void_pointer<element_t*>::value, "Cannot launder void pointers");

	#if defined(__has_builtin)
		#if __has_builtin(__builtin_launder)
			return __builtin_launder(p_ptr);
		#endif
	#endif
			return p_ptr;
		}
	};

	template <typename type_t> auto launder(type_t* p_ptr) noexcept -> type_t*
	{
		return claunder<type_t>::launder(p_ptr);
	}

	class claunder_exc : public std::runtime_error
	{
	public:
		explicit claunder_exc(const std::string& p_message) : std::runtime_error("Error: " + p_message) {}
	};

}	 // namespace utils

#endif	  // CLAUNDER_HPP