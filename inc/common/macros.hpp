#ifndef MACROS_HPP
#define MACROS_HPP

// NOLINTBEGIN
#define cr_decltype(in_var) const decltype(in_var)&
#define r_decltype(in_var) decltype(in_var)&
// NOLINTEND

inline auto TRAP_DBG() -> void
{
#if defined(NDEBUG)
	return;
#endif

	return;
	__asm__("int $3");
}

#include "loggers.hpp"

template <class> extern constexpr bool always_false_v = false;

extern const UNIQUE_PTR_LOGGER g_logger;

inline auto LOG_CRITICAL([[maybe_unused]] std::string_view in_msg) -> void
{
#if not defined(F_CRITICAL)
	return;
#endif

	g_logger->critical("{}", in_msg);
	TRAP_DBG();
}

inline auto LOG_ERROR([[maybe_unused]] std::string_view in_msg) -> void
{
#if not defined(F_ERROR)
	return;
#endif

	g_logger->error("{}", in_msg);
	TRAP_DBG();
}

inline auto LOG_WARN([[maybe_unused]] std::string_view in_msg) -> void
{
#if not defined(F_WARN)
	return;
#endif

	g_logger->warn("{}", in_msg);
}

#endif // MACROS_HPP
