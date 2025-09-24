#pragma once
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

	// MACRO_CONSTEXPR_FUNC
	#if MACRO_CXX14_ENABLED
		#define MACRO_CONSTEXPR_FUNC constexpr
	#else
		#define MACRO_CONSTEXPR_FUNC inline
	#endif

#endif	  // UTILS_MACROS_HPP
