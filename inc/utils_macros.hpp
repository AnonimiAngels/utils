#ifndef UTILS_MACROS_HPP
#define UTILS_MACROS_HPP

// MACRO_CXX14_ENABLED
#if __cplusplus >= 201402L
	#define MACRO_CXX14_ENABLED 1
#else
	#define MACRO_CXX14_ENABLED 0
#endif

// MACRO_CXX17_ENABLED
#if __cplusplus >= 201703L
	#define MACRO_CXX17_ENABLED 1
#else
	#define MACRO_CXX17_ENABLED 0
#endif

// MACRO_CXX20_ENABLED
#if __cplusplus >= 202002L
	#define MACRO_CXX20_ENABLED 1
#else
	#define MACRO_CXX20_ENABLED 0
#endif

// MACRO_CXX23_ENABLED
#if __cplusplus >= 202302L
	#define MACRO_CXX23_ENABLED 1
#else
	#define MACRO_CXX23_ENABLED 0
#endif

// MACRO_NODISCARD
#if MACRO_CXX17_ENABLED
	#define MACRO_NODISCARD [[nodiscard]]
#else
	#if defined(__has_cpp_attribute)
		#if __has_cpp_attribute(nodiscard)
			#define MACRO_NODISCARD [[nodiscard]]
		#else
			#define MACRO_NODISCARD
		#endif
	#else
		#define MACRO_NODISCARD
	#endif
#endif

// MACRO_MAYBE_UNUSED
#if MACRO_CXX17_ENABLED
	#define MACRO_MAYBE_UNUSED [[maybe_unused]]
#else
	#if defined(__has_cpp_attribute)
		#if __has_cpp_attribute(maybe_unused)
			#define MACRO_MAYBE_UNUSED [[maybe_unused]]
		#else
			#define MACRO_MAYBE_UNUSED
		#endif
	#else
		#define MACRO_MAYBE_UNUSED
	#endif
#endif

// MACRO_REQUIRE_CONSTRAINTS
#if MACRO_CXX20_ENABLED
	#define MACRO_REQUIRE_CONSTRAINTS require
#else
	#define MACRO_REQUIRE_CONSTRAINTS
#endif

// MACRO_CONSTEXPR_FUNC
#if MACRO_CXX14_ENABLED
	#define MACRO_CONSTEXPR_FUNC constexpr
#else
	#define MACRO_CONSTEXPR_FUNC inline
#endif

// MACRO_EXCEPTIONS_ENABLED
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
	#define MACRO_EXCEPTIONS_ENABLED 1
#else
	#define MACRO_EXCEPTIONS_ENABLED 0
#endif

// MACRO_THROW
#if MACRO_EXCEPTIONS_ENABLED
	#define MACRO_GET_3RD_ARG(arg1, arg2, arg3, ...) arg3
	#define MACRO_THROW_CHOOSER(...) MACRO_GET_3RD_ARG(__VA_ARGS__, MACRO_THROW_2, MACRO_THROW_1)

	#define MACRO_THROW_1(exception) throw exception
	#define MACRO_THROW_2(exception, error) throw exception(error)

	#define MACRO_THROW(...) MACRO_THROW_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
#else
	#include <cstdio>
	#include <cstdlib>

	#define MACRO_GET_3RD_ARG(arg1, arg2, arg3, ...) arg3
	#define MACRO_THROW_CHOOSER(...) MACRO_GET_3RD_ARG(__VA_ARGS__, MACRO_THROW_2, MACRO_THROW_1)

	#define MACRO_THROW_1(exception)                                                                                                                 \
		do                                                                                                                                           \
		{                                                                                                                                            \
			std::println("Fatal error, {}", #exception);                                                                                             \
			std::abort();                                                                                                                            \
		} while (0)

	#define MACRO_THROW_2(exception, error)                                                                                                          \
		do                                                                                                                                           \
		{                                                                                                                                            \
			std::println("Fatal error: {}", error);                                                                                                  \
			std::abort();                                                                                                                            \
		} while (0)

	#define MACRO_THROW(...) MACRO_THROW_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
#endif

#endif	  // UTILS_MACROS_HPP
