#pragma once
#ifndef TEST_COMMON_HPP
	#define TEST_COMMON_HPP

	#include <chrono>
	#include <exception>
	#include <functional>
	#include <memory>
	#include <string>

	#include "cmemory.hpp"
	#include "expected.hpp"
	#include "filesystem.hpp"
	#include "format.hpp"
	#include "loggers.hpp"
	#include "spdlog/sinks/stdout_color_sinks.h"
	#include "spdlog/spdlog.h"
	#include "string_view.hpp"
	#include "test_macros.hpp"

namespace test_common
{
	template <typename T, typename E = std::string> using test_expected = utils::expected<T, E>;

	class test_runner
	{
	public:
		using self_t = test_runner;

	private:
		std::unique_ptr<utils::logger> m_logger;
		std::string m_test_name;
		std::chrono::steady_clock::time_point m_start_time;
		bool m_logging_enabled;

	public:
		explicit test_runner(const std::string_view& p_test_name, bool p_enable_logging = false)
			: m_test_name(p_test_name), m_logging_enabled(p_enable_logging)
		{
			if (m_logging_enabled)
			{
				m_logger = A_MAKE_UNIQUE_LOGGER;
			}
			m_start_time = std::chrono::steady_clock::now();
		}

		~test_runner()
		{
			if (m_logging_enabled && m_logger)
			{
				auto end_time = std::chrono::steady_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - m_start_time);
				m_logger->info("Test '{}' completed in {} μs", m_test_name, duration.count());
				spdlog::drop(std::format("test_{}", m_test_name));
			}
		}

		template <typename... args_t> auto log_info(const utils::logger::fmt_str_t<args_t...>& p_format, args_t&&... p_args) -> void
		{
			if (m_logging_enabled && m_logger)
			{
				m_logger->info(p_format, std::forward<args_t>(p_args)...);
			}
		}

		template <typename... args_t> auto log_debug(const utils::logger::fmt_str_t<args_t...>& p_format, args_t&&... p_args) -> void
		{
			if (m_logging_enabled && m_logger)
			{
				m_logger->debug(p_format, std::forward<args_t>(p_args)...);
			}
		}

		template <typename... args_t> auto log_error(const utils::logger::fmt_str_t<args_t...>& p_format, args_t&&... p_args) -> void
		{
			if (m_logging_enabled && m_logger)
			{
				m_logger->error(p_format, std::forward<args_t>(p_args)...);
			}
		}

		template <typename T, typename E>
		auto assert_expected_value(const test_expected<T, E>& p_result, const T& p_expected_value, const std::string& p_context = "") -> void
		{
			std::string full_context = p_context.empty() ? m_test_name : std::format("{}: {}", m_test_name, p_context);

			if (!p_result.has_value())
			{
				std::string error_msg = std::format("Expected value but got error in {}: {}", full_context, p_result.error());
				log_error("{}", error_msg);
				throw std::runtime_error(error_msg);
			}

			if (p_result.value() != p_expected_value)
			{
				std::string error_msg =
					std::format("Value mismatch in {}: expected '{}', got '{}'", full_context, p_expected_value, p_result.value());
				log_error("{}", error_msg);
				throw std::runtime_error(error_msg);
			}

			log_debug("Expected value assertion passed in {}", full_context);
		}

		template <typename T, typename E>
		auto assert_expected_error(const test_expected<T, E>& p_result,
								   const std::string& p_expected_error_substr = "",
								   const std::string& p_context				  = "") -> void
		{
			std::string full_context = p_context.empty() ? m_test_name : std::format("{}: {}", m_test_name, p_context);

			if (p_result.has_value())
			{
				std::string error_msg = std::format("Expected error but got value in {}: {}", full_context, p_result.value());
				log_error("{}", error_msg);
				throw std::runtime_error(error_msg);
			}

			if (!p_expected_error_substr.empty())
			{
				std::string error_str = std::format("{}", p_result.error());
				if (error_str.find(p_expected_error_substr) == std::string::npos)
				{
					std::string error_msg = std::format("Error message mismatch in {}: expected to contain '{}', got '{}'",
														full_context,
														p_expected_error_substr,
														error_str);
					log_error("{}", error_msg);
					throw std::runtime_error(error_msg);
				}
			}

			log_debug("Expected error assertion passed in {}", full_context);
		}

		template <typename func_t> auto run_test_case(const std::string& p_case_name, const func_t& p_test_func) -> void
		{
			std::string full_case_name	= std::format("{}::{}", m_test_name, p_case_name);
			const auto& executable_name = fs::read_symlink("/proc/self/exe").filename();

			log_info("Running test case: {} {}", executable_name.string(), full_case_name);

			try
			{
				p_test_func();
				log_info("Test case passed: {}", full_case_name);
			}
			catch (const std::exception& e)
			{
				log_error("Test case failed: {} - {}", full_case_name, e.what());
				throw std::runtime_error(std::format("Test case '{}' failed: {}", full_case_name, e.what()));
			}
		}

		template <typename T> auto assert_equals(const T& p_actual, const T& p_expected, const std::string& p_context = "") -> void
		{
			std::string full_context = p_context.empty() ? m_test_name : std::format("{}: {}", m_test_name, p_context);

			if (p_actual != p_expected)
			{
				std::string error_msg = std::format("Assertion failed in {}: expected '{}', got '{}'", full_context, p_expected, p_actual);
				log_error("{}", error_msg);
				throw std::runtime_error(error_msg);
			}

			log_debug("Equality assertion passed in {}", full_context);
		}

		auto assert_true(bool p_condition, const std::string& p_message = "", const std::string& p_context = "") -> void
		{
			std::string full_context = p_context.empty() ? m_test_name : std::format("{}: {}", m_test_name, p_context);

			if (!p_condition)
			{
				std::string error_msg = p_message.empty() ? std::format("Assertion failed in {}: expected true", full_context)
														  : std::format("Assertion failed in {}: {}", full_context, p_message);
				log_error("{}", error_msg);
				throw std::runtime_error(error_msg);
			}

			log_debug("Boolean assertion passed in {}", full_context);
		}

		auto assert_false(bool p_condition, const std::string& p_message = "", const std::string& p_context = "") -> void
		{
			assert_true(!p_condition, p_message.empty() ? "expected false" : p_message, p_context);
		}
	};

	// Convenience function for creating test runners with logging
	inline auto create_test_runner(const std::string& p_test_name, bool p_enable_logging = false) -> std::unique_ptr<test_runner>
	{
		return std::make_unique<test_runner>(p_test_name, p_enable_logging);
	}

	// Utility function for creating expected results
	template <typename T, typename E = std::string> auto make_test_expected(T&& p_value) -> test_expected<T, E>
	{
		return utils::expected<T, E>(std::forward<T>(p_value));
	}

	template <typename T, typename E = std::string> auto make_test_error(E&& p_error) -> test_expected<T, E>
	{
		return utils::make_unexpected(std::forward<E>(p_error));
	}
}	 // namespace test_common

#endif	  // TEST_COMMON_HPP
