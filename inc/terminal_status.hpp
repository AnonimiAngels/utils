#ifndef TERMINAL_STATUS_HPP
#define TERMINAL_STATUS_HPP

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

#include <sys/ioctl.h>
#include <unistd.h>

#include "format.hpp"
#include "precision_timer.hpp"
#include "utils_manip.hpp"

namespace terminal
{
	/**
	 * @brief Terminal status line manager for persistent bottom lines
	 *
	 * Manages terminal output to keep status lines at the bottom while
	 * allowing normal output to scroll above them.
	 */
	class status_manager
	{
	public:
		using self_t = status_manager;

	private:
		utils::precision_timer<std::chrono::milliseconds> m_timer;
		std::map<std::int32_t, std::string> m_lines;
		std::int32_t m_requested_lines;
		std::int32_t m_term_rows;
		std::int32_t m_term_cols;
		bool m_initialized;
		static status_manager* s_active_instance;
		static std::atomic<bool> s_resize_pending;
		struct sigaction m_old_sigwinch_handler;
		bool m_signal_handler_set;
		std::uint32_t m_last_elapsed_ms;

	public:
		/**
		 * @brief Destructor restores terminal state
		 */
		~status_manager()
		{
			if (m_initialized)
			{
				deinit();
			}
			if (m_signal_handler_set)
			{
				restore_signal_handler();
			}
			if (s_active_instance == this)
			{
				s_active_instance = nullptr;
			}
		}

		/**
		 * @brief Default constructor
		 */
		status_manager()
			: m_requested_lines(0), m_term_rows(0), m_term_cols(0), m_initialized(false), m_signal_handler_set(false), m_last_elapsed_ms(0)
		{
		}

		status_manager(const self_t&)			 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		/**
		 * @brief Move constructor
		 */
		status_manager(self_t&& p_other) noexcept
			: m_lines(std::move(p_other.m_lines)),
			  m_requested_lines(p_other.m_requested_lines),
			  m_term_rows(p_other.m_term_rows),
			  m_term_cols(p_other.m_term_cols),
			  m_initialized(p_other.m_initialized),
			  m_old_sigwinch_handler(p_other.m_old_sigwinch_handler),
			  m_signal_handler_set(p_other.m_signal_handler_set),
			  m_last_elapsed_ms(p_other.m_last_elapsed_ms)
		{
			p_other.m_initialized		 = false;
			p_other.m_signal_handler_set = false;
			if (s_active_instance == &p_other)
			{
				s_active_instance = this;
			}
		}

		/**
		 * @brief Move assignment operator
		 */
		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				if (m_initialized)
				{
					deinit();
				}
				if (m_signal_handler_set)
				{
					restore_signal_handler();
				}
				m_lines						 = std::move(p_other.m_lines);
				m_requested_lines			 = p_other.m_requested_lines;
				m_term_rows					 = p_other.m_term_rows;
				m_term_cols					 = p_other.m_term_cols;
				m_initialized				 = p_other.m_initialized;
				m_old_sigwinch_handler		 = p_other.m_old_sigwinch_handler;
				m_signal_handler_set		 = p_other.m_signal_handler_set;
				m_last_elapsed_ms			 = p_other.m_last_elapsed_ms;
				p_other.m_initialized		 = false;
				p_other.m_signal_handler_set = false;
				if (s_active_instance == &p_other)
				{
					s_active_instance = this;
				}
			}
			return *this;
		}

	private:
		/**
		 * @brief Static signal handler for SIGWINCH
		 */
		static auto handle_resize(std::int32_t /*p_sig*/) -> void { s_resize_pending.store(true); }

		/**
		 * @brief Setup signal handler for terminal resize
		 */
		auto setup_signal_handler() -> void
		{
			if (m_signal_handler_set)
			{
				return;
			}

			s_active_instance = this;

			struct sigaction sa;
			std::memset(&sa, 0, sizeof(sa));
			sa.sa_handler = handle_resize;
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = SA_RESTART;

			if (sigaction(SIGWINCH, &sa, &m_old_sigwinch_handler) == 0)
			{
				m_signal_handler_set = true;
			}
		}

		/**
		 * @brief Restore original signal handler
		 */
		auto restore_signal_handler() -> void
		{
			if (!m_signal_handler_set)
			{
				return;
			}

			sigaction(SIGWINCH, &m_old_sigwinch_handler, nullptr);
			m_signal_handler_set = false;
		}

		/**
		 * @brief Check and handle pending resize
		 */
		auto check_resize() -> void
		{
			if (s_resize_pending.exchange(false))
			{
				refresh();
			}
		}

		/**
		 * @brief Update terminal dimensions
		 */
		auto update_term_size() -> void
		{
			struct winsize ws;
			if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
			{
				m_term_rows = static_cast<std::int32_t>(ws.ws_row);
				m_term_cols = static_cast<std::int32_t>(ws.ws_col);
			}
		}

		/**
		 * @brief Initialize terminal for status lines
		 */
		auto init() -> void
		{
			if (m_initialized)
			{
				return;
			}

			update_term_size();
			setup_signal_handler();

			// Ensure we have space for status lines
			for (std::int32_t idx_for = 0; idx_for < m_requested_lines; ++idx_for)
			{
				std::print("\n");
			}

			// Save cursor and set scrollable region
			std::print("\033[s");										  // Save cursor position
			std::print("\033[0;{}r", m_term_rows - m_requested_lines);	  // Set scroll region
			std::print("\033[u");										  // Restore cursor position

			// Move cursor up to compensate for newlines
			if (m_requested_lines > 0)
			{
				std::print("\033[{}A", m_requested_lines);
			}

			std::fflush(stdout);
			m_initialized = true;
		}

		/**
		 * @brief Deinitialize terminal
		 */
		auto deinit() -> void
		{
			if (!m_initialized)
			{
				return;
			}

			// Reset scrollable region and clear status lines
			std::print("\033[s");					  // Save cursor position
			std::print("\033[0;{}r", m_term_rows);	  // Reset scroll region

			// Clear status lines
			for (std::int32_t idx_for = 0; idx_for < m_requested_lines; ++idx_for)
			{
				std::int32_t row = m_term_rows - m_requested_lines + idx_for + 1;
				std::print("\033[{};0H", row);	  // Move to line
				std::print("\033[0K");			  // Clear line
			}

			std::print("\033[u");	 // Restore cursor position
			std::fflush(stdout);
			m_initialized = false;
		}

		/**
		 * @brief Redraw all status lines
		 */
		auto redraw_lines() -> void
		{
			if (!m_initialized)
			{
				return;
			}

			check_resize();

			std::print("\033[s");	 // Save cursor position

			for (std::int32_t idx_for = 1; idx_for <= m_requested_lines; ++idx_for)
			{
				std::int32_t row = m_term_rows - m_requested_lines + idx_for;
				std::print("\033[{};0H", row);	  // Move to line
				std::print("\033[0K");			  // Clear line

				auto it = m_lines.find(idx_for);
				if (it != m_lines.end())
				{
					// Truncate if too long
					std::string text = it->second;
					if (static_cast<std::int32_t>(text.length()) > m_term_cols)
					{
						text = text.substr(0, static_cast<std::size_t>(m_term_cols));
					}
					std::print("{}", text);
				}
			}

			std::print("\033[u");	 // Restore cursor position
			std::fflush(stdout);
		}

	public:
		/**
		 * @brief Request number of status lines at bottom
		 */
		auto request_lines(std::int32_t p_count) -> void
		{
			if (p_count < 0)
			{
				throw std::runtime_error("Line count must be non-negative");
			}

			if (m_initialized)
			{
				deinit();
			}

			m_requested_lines = p_count;
			m_lines.clear();

			if (p_count > 0)
			{
				init();
			}
		}

		/**
		 * @brief Set text for a specific line
		 */
		auto set_line(std::int32_t p_line, const std::string& p_text) -> void
		{
			if (p_line < 1 || p_line > m_requested_lines)
			{
				throw std::out_of_range(std::format("Line {} out of range (1-{})", p_line, m_requested_lines));
			}

			m_lines[p_line] = p_text;
			redraw_lines();
		}

		/**
		 * @brief Clear a specific line
		 */
		auto clear_line(std::int32_t p_line) -> void
		{
			if (p_line < 1 || p_line > m_requested_lines)
			{
				throw std::out_of_range(std::format("Line {} out of range (1-{})", p_line, m_requested_lines));
			}

			m_lines.erase(p_line);
			redraw_lines();
		}

		/**
		 * @brief Clear all lines
		 */
		auto clear_all() -> void
		{
			m_lines.clear();
			redraw_lines();
		}

		/**
		 * @brief Refresh display (e.g., after terminal resize)
		 */
		auto refresh() -> void
		{
			update_term_size();
			if (m_initialized)
			{
				deinit();
				init();
				redraw_lines();
			}
		}

		/**
		 * @brief Create a progress bar string
		 */
		auto make_progress_bar(std::int32_t p_current, std::int32_t p_total, char p_bar_char = '|', char p_empty_char = ' ') -> std::string
		{
			if (p_total <= 0)
			{
				return "";
			}

			if (!m_timer.is_started())
			{
				m_timer.start();
				m_last_elapsed_ms = 0;
			}

			const std::uint32_t total_elapsed_ms = m_timer.get_elapsed().count();
			const std::uint32_t delta_ms		 = total_elapsed_ms - m_last_elapsed_ms;
			m_last_elapsed_ms					 = total_elapsed_ms;

			std::int32_t perc = (p_current * 100) / p_total;

			std::string time_info = "";
			if (p_current > 0 && total_elapsed_ms > 0)
			{
				const double avg_ms_per_item  = static_cast<double>(total_elapsed_ms) / static_cast<double>(p_current);
				const std::uint32_t remain_ms = static_cast<std::uint32_t>(avg_ms_per_item * static_cast<double>(p_total - p_current));

				time_info = std::format(" | Δ{} ETA:{}", ms_to_string(delta_ms), ms_to_string(remain_ms));
			}

			std::string suffix	 = std::format(" {}/{} ({}%){}", p_current, p_total, perc, time_info);
			std::int32_t bar_len = m_term_cols - static_cast<std::int32_t>(suffix.length()) - 2;
			bar_len				 = std::max(bar_len, 0);

			std::int32_t filled = (perc * bar_len) / 100;

			std::string bar = "[";
			for (std::int32_t idx_for = 0; idx_for < filled; ++idx_for)
			{
				bar += p_bar_char;
			}
			for (std::int32_t idx_for = filled; idx_for < bar_len; ++idx_for)
			{
				bar += p_empty_char;
			}
			bar += "]";
			bar += suffix;

			return bar;
		}

		/**
		 * @brief Set a line to show a progress bar
		 */
		auto set_progress(std::int32_t p_line, std::int32_t p_current, std::int32_t p_total, char p_bar_char = '|', char p_empty_char = ' ') -> void
		{
			set_line(p_line, make_progress_bar(p_current, p_total, p_bar_char, p_empty_char));
		}

		/**
		 * @brief Process pending resize events
		 */
		auto process_events() -> void { check_resize(); }
	};

	// Static member definitions
	status_manager* status_manager::s_active_instance = nullptr;
	std::atomic<bool> status_manager::s_resize_pending(false);

}	 // namespace terminal

#endif	  // TERMINAL_STATUS_HPP