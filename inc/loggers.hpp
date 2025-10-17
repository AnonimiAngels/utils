#pragma once
#ifndef LOGGERS_HPP
	#define LOGGERS_HPP

/**
 * @file loggers.hpp
 * @author anonimi_angels (gutsulyak.vladyslav2000@gmail.com)
 * @brief Logger utility class using spdlog
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

	#define SPDLOG_HEADER_ONLY

	#include <spdlog/logger.h>
	#include <spdlog/sinks/basic_file_sink.h>
	#include <spdlog/sinks/stdout_color_sinks.h>

	#include "cmemory.hpp"
	#include "std/string_view.hpp"

	#define UNIQUE_PTR_LOGGER std::unique_ptr<utils::logger>
	#define A_MAKE_UNIQUE_LOGGER std::unique_ptr<utils::logger>(new utils::logger(__FUNCTION__))
	#define A_DEFINE_UNIQUE_LOGGER(in_name) UNIQUE_PTR_LOGGER(in_name) = std::unique_ptr<utils::logger>(new utils::logger(__FUNCTION__))

namespace utils
{
	/**
	 * @brief Logger class that uses spdlog for logging functionality
	 */
	class logger
	{
	public:
		using self_t								  = logger;
		template <typename... args_t> using fmt_str_t = spdlog::format_string_t<args_t...>;

	private:
		std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> m_console_sink;
		std::shared_ptr<spdlog::sinks::basic_file_sink_mt> m_file_sink;
		std::shared_ptr<spdlog::logger> m_logger;
		bool m_enabled = true;

	public:
		/**
		 * @brief Construct logger with entity name only
		 * @param p_entity Logger entity name
		 */
		logger(std::string_view p_entity)
			: m_console_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()),
			  m_file_sink(nullptr),
			  m_logger(std::make_shared<spdlog::logger>(p_entity.cbegin(), m_console_sink))
		{
			console_sync();
			logger_sync();
		}

		/**
		 * @brief Construct logger with entity name and log file
		 * @param p_entity Logger entity name
		 * @param p_log_file Log file path
		 * @param p_console_sink Enable console sink flag
		 */
		logger(std::string_view p_entity, std::string_view p_log_file, bool p_console_sink = false)
			: m_console_sink(p_console_sink ? std::make_shared<spdlog::sinks::stdout_color_sink_mt>() : nullptr),
			  m_file_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(p_log_file.cbegin(), true)),
			  m_logger(nullptr)
		{
			if (p_console_sink)
			{
				m_logger = std::make_shared<spdlog::logger>(p_entity.cbegin(), spdlog::sinks_init_list{m_console_sink, m_file_sink});
				console_sync();
			}
			else
			{
				m_logger = std::make_shared<spdlog::logger>(p_entity.cbegin(), m_file_sink);
			}
			logger_sync();
		}

		/**
		 * @brief Get logger instance
		 * @return Shared pointer to spdlog logger
		 */
		auto operator()() noexcept -> std::shared_ptr<spdlog::logger> { return m_logger; }

		/**
		 * @brief Arrow operator for accessing logger
		 * @return Shared pointer to spdlog logger
		 */
		auto operator->() noexcept -> std::shared_ptr<spdlog::logger> { return m_logger; }

		/**
		 * @brief Enable logging
		 */
		auto enable() noexcept -> void { m_enabled = true; }

		/**
		 * @brief Disable logging
		 */
		auto disable() noexcept -> void { m_enabled = false; }

		/**
		 * @brief Log trace level message
		 * @param p_fmt Format string
		 * @param p_args Arguments for format string
		 */
		template <typename... args_t> auto trace(fmt_str_t<args_t...> p_fmt, args_t&&... p_args) const noexcept -> void
		{
			if (!m_enabled)
			{
				return;
			}
			m_logger->trace(p_fmt, std::forward<args_t>(p_args)...);
		}

		/**
		 * @brief Log debug level message
		 * @param p_fmt Format string
		 * @param p_args Arguments for format string
		 */
		template <typename... args_t> auto debug(fmt_str_t<args_t...> p_fmt, args_t&&... p_args) const noexcept -> void
		{
			if (!m_enabled)
			{
				return;
			}
			m_logger->debug(p_fmt, std::forward<args_t>(p_args)...);
		}

		/**
		 * @brief Log info level message
		 * @param p_fmt Format string
		 * @param p_args Arguments for format string
		 */
		template <typename... args_t> auto info(fmt_str_t<args_t...> p_fmt, args_t&&... p_args) const noexcept -> void
		{
			if (!m_enabled)
			{
				return;
			}
			m_logger->info(p_fmt, std::forward<args_t>(p_args)...);
		}

		/**
		 * @brief Log warning level message
		 * @param p_fmt Format string
		 * @param p_args Arguments for format string
		 */
		template <typename... args_t> auto warn(fmt_str_t<args_t...> p_fmt, args_t&&... p_args) const noexcept -> void
		{
			if (!m_enabled)
			{
				return;
			}
			m_logger->warn(p_fmt, std::forward<args_t>(p_args)...);
		}

		/**
		 * @brief Log error level message
		 * @param p_fmt Format string
		 * @param p_args Arguments for format string
		 */
		template <typename... args_t> auto error(fmt_str_t<args_t...> p_fmt, args_t&&... p_args) const noexcept -> void
		{
			if (!m_enabled)
			{
				return;
			}
			m_logger->error(p_fmt, std::forward<args_t>(p_args)...);
		}

		/**
		 * @brief Log critical level message
		 * @param p_fmt Format string
		 * @param p_args Arguments for format string
		 */
		template <typename... args_t> auto critical(fmt_str_t<args_t...> p_fmt, args_t&&... p_args) const noexcept -> void
		{
			if (!m_enabled)
			{
				return;
			}
			m_logger->critical(p_fmt, std::forward<args_t>(p_args)...);
		}

	private:
		/**
		 * @brief Configure console sink colors
		 */
		auto console_sync() -> void
		{
			const char* white		= "\033[0m";
			const char* cyan		= "\033[36m";
			const char* green		= "\033[32m";
			const char* yellow_bold = "\033[1;33m";
			const char* red_bold	= "\033[1;31m";
			const char* bold_on_red = "\033[1;31m";

			m_console_sink->set_color_mode(spdlog::color_mode::always);
			m_console_sink->set_color(spdlog::level::trace, white);
			m_console_sink->set_color(spdlog::level::debug, cyan);
			m_console_sink->set_color(spdlog::level::info, green);
			m_console_sink->set_color(spdlog::level::warn, yellow_bold);
			m_console_sink->set_color(spdlog::level::err, red_bold);
			m_console_sink->set_color(spdlog::level::critical, bold_on_red);
			m_console_sink->set_color(spdlog::level::off, white);
		}

		/**
		 * @brief Configure logger pattern
		 */
		auto logger_sync() -> void
		{
			m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%t] %v");
			m_enabled = true;
		}
	};

}	 // namespace utils

#endif