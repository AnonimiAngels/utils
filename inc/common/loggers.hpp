#pragma once
#ifndef LOGGERS_HPP
#define LOGGERS_HPP

/**
 * @file loggers.hpp
 * @author anonimi_angels (gutsulyak.vladyslav2000@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-11-13
 *
 * @copyright
 *The MIT License (MIT)
 *Copyright (c) 2024 anonimi_angels
 *Permission is hereby granted, free of charge, to any person obtaining a copy
 *of this software and associated documentation files (the "Software"), to deal
 *in the Software without restriction, including without limitation the rights
 *to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *copies of the Software, and to permit persons to whom the Software is
 *furnished to do so, subject to the following conditions:
 *The above copyright notice and this permission notice shall be included in all
 *copies or substantial portions of the Software.
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *SOFTWARE.
 *
 */

// External includes
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/fmt/ranges.h>

// Standard includes
#include <memory>
#include <string>
#include <string_view>

#define UNIQUE_PTR_LOGGER std::unique_ptr<utils::logger>
#define MAKE_UNIQUE_LOGGER std::make_unique<utils::logger>
#define A_MAKE_UNIQUE_LOGGER MAKE_UNIQUE_LOGGER(__FUNCTION__)
#define A_DEFINE_UNIQUE_LOGGER(in_name) UNIQUE_PTR_LOGGER(in_name) = A_MAKE_UNIQUE_LOGGER

#define F_CRITICAL 1
#define F_ERROR 1
#define F_INFO 1
#define F_TRACE 1
#define F_WARNING 1

namespace utils
{
	/**
	 * @brief A logger class that uses spdlog
	 */
	class logger
	{
	  private:
		spdlog::level::level_enum m_level = spdlog::level::trace;
		std::shared_ptr<spdlog::logger> m_logger;
		std::shared_ptr<spdlog::sinks::basic_file_sink_mt> m_file_sink;
		std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> m_console_sink;

	  public:
		/**
		 * @brief Construct a new logger object
		 * @param entity The entity name
		 * @param log_file The log file name
		 * @note The log file will be created in the current directory under logs
		 */
		logger(std::string_view entity, std::string_view log_file, bool f_console_sink = false) noexcept
		{
			m_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file.cbegin(), true);

			if (f_console_sink)
			{
				m_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
				m_logger	   = std::make_shared<spdlog::logger>(entity.cbegin(), spdlog::sinks_init_list{m_console_sink, m_file_sink});
			}
			else { m_logger = std::make_shared<spdlog::logger>(entity.cbegin(), m_file_sink); }
			m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%t] %v"); // Example: [2021-09-01 12:00:00.000000] [INFO] [main] [thread_id] message
		}

		/**
		 * @brief Construct a new logger object
		 * @param entity The entity name
		 * @note The log file will be created in the current directory under logs
		 */
		logger(std::string_view entity) noexcept
		{
			m_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

			m_logger = std::make_shared<spdlog::logger>(entity.cbegin(), m_console_sink);
			m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%t] %v"); // Example: [2021-09-01 12:00:00.000000] [INFO] [main] [thread_id] message
		}

		/**
		 * @brief Function to log a message at the trace level
		 */
		template <typename... in_args_t> auto trace([[maybe_unused]] fmt::format_string<in_args_t...> fmt, [[maybe_unused]] in_args_t&&... args) -> void
		{
#if defined(F_TRACE)
			m_level							  = spdlog::level::trace;
			spdlog::level::level_enum g_level = spdlog::get_level();

			if (m_level >= g_level)
			{
				m_logger->set_level(m_level);
				m_logger->trace(fmt, std::forward<in_args_t>(args)...);
			}
#endif
		}

		/**
		 * @brief Function to log a message at the debug level
		 */
		template <typename... in_args_t> auto debug([[maybe_unused]] fmt::format_string<in_args_t...> fmt, [[maybe_unused]] in_args_t&&... args) -> void
		{
#if not defined(NDEBUG)

			m_level							  = spdlog::level::debug;
			spdlog::level::level_enum g_level = spdlog::get_level();

			if (m_level >= g_level)
			{
				m_logger->set_level(m_level);
				m_logger->debug(fmt, std::forward<in_args_t>(args)...);
			}
#endif
		}

		/**
		 * @brief Function to log a message at the info level
		 */
		template <typename... in_args_t> auto info([[maybe_unused]] fmt::format_string<in_args_t...> fmt, [[maybe_unused]] in_args_t&&... args) -> void
		{
#if defined(F_INFO)

			m_level							  = spdlog::level::info;
			spdlog::level::level_enum g_level = spdlog::get_level();

			if (m_level >= g_level)
			{
				m_logger->set_level(m_level);
				m_logger->info(fmt, std::forward<in_args_t>(args)...);
			}
#endif
		}

		/**
		 * @brief Function to log a message at the warning level
		 */
		template <typename... in_args_t> auto warn([[maybe_unused]] fmt::format_string<in_args_t...> fmt, [[maybe_unused]] in_args_t&&... args) -> void
		{
#if defined(F_WARNING)
			m_level							  = spdlog::level::warn;
			spdlog::level::level_enum g_level = spdlog::get_level();

			if (m_level >= g_level)
			{
				m_logger->set_level(m_level);
				m_logger->warn(fmt, std::forward<in_args_t>(args)...);
			}
#endif
		}

		/**
		 * @brief Function to log a message at the error level
		 */
		template <typename... in_args_t> auto error([[maybe_unused]] fmt::format_string<in_args_t...> fmt, [[maybe_unused]] in_args_t&&... args) -> void
		{
#if defined(F_ERROR)

			m_level							  = spdlog::level::err;
			spdlog::level::level_enum g_level = spdlog::get_level();

			if (m_level >= g_level)
			{
				m_logger->set_level(m_level);
				m_logger->error(fmt, std::forward<in_args_t>(args)...);
			}
#endif
		}

		/**
		 * @brief Function to log a message at the critical level
		 */
		template <typename... in_args_t> auto critical([[maybe_unused]] fmt::format_string<in_args_t...> fmt, [[maybe_unused]] in_args_t&&... args) -> void
		{
#if defined(F_CRITICAL)

			m_level							  = spdlog::level::critical;
			spdlog::level::level_enum g_level = spdlog::get_level();

			if (m_level >= g_level)
			{
				m_logger->set_level(m_level);
				m_logger->critical(fmt, std::forward<in_args_t>(args)...);
			}
#endif
		}

	  private:
	}; // namespace utils
} // namespace utils

#endif // LOGGERS_HPP
