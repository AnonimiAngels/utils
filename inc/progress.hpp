#pragma once
#ifndef PROGRESS_HPP
	#define PROGRESS_HPP

	#include <atomic>
	#include <cstdio>
	#include <mutex>
	#include <stdexcept>
	#include <string>

	#include "format.hpp"

class progress
{
public:
	using self_t = progress;

	progress() = default;

	progress(const self_t&)					 = delete;
	auto operator=(const self_t&) -> self_t& = delete;

	progress(self_t&& p_other) noexcept
		: m_progress(p_other.m_progress.load()),
		  m_total(p_other.m_total.load()),
		  m_fill_char(p_other.m_fill_char.load()),
		  m_is_incremental(p_other.m_is_incremental.load()),
		  m_is_verbose(p_other.m_is_verbose.load())
	{
	}

	auto operator=(self_t&& p_other) noexcept -> self_t&
	{
		if (this != &p_other)
		{
			m_progress.store(p_other.m_progress.load());
			m_total.store(p_other.m_total.load());
			m_fill_char.store(p_other.m_fill_char.load());
			m_is_incremental.store(p_other.m_is_incremental.load());
			m_is_verbose.store(p_other.m_is_verbose.load());
		}
		return *this;
	}

	~progress() = default;

	auto set_progress(std::size_t p_progress) -> void { m_progress.store(p_progress); }

	auto set_total(std::size_t p_total) -> void { m_total.store(p_total); }

	auto set_fill_char(char p_fill_char) -> void { m_fill_char.store(p_fill_char); }

	auto set_is_incremental(bool p_is_incremental) -> void { m_is_incremental.store(p_is_incremental); }

	auto set_is_verbose(bool p_is_verbose) -> void { m_is_verbose.store(p_is_verbose); }

	auto get_progress() const -> std::size_t { return m_progress.load(); }

	auto get_total() const -> std::size_t { return m_total.load(); }

	auto get_fill_char() const -> char { return m_fill_char.load(); }

	auto get_is_incremental() const -> bool { return m_is_incremental.load(); }

	auto get_is_verbose() const -> bool { return m_is_verbose.load(); }

	auto print_progress() -> void
	{
		if (m_is_incremental.load())
		{
			m_progress.fetch_add(1);
		}

		if (!m_is_verbose.load())
		{
			return;
		}

		const auto current_progress = m_progress.load();
		const auto current_total	= m_total.load();
		const auto current_fill		= m_fill_char.load();

		if (current_total == 0)
		{
			return;
		}

		const auto percentage = static_cast<double>(current_progress) / static_cast<double>(current_total) * 100.0;
		const auto num_hashes = static_cast<std::size_t>(percentage / 2.0);
		const auto num_spaces = 50 - num_hashes;

		std::lock_guard<std::mutex> lock(m_print_mutex);
		const auto filled_bar = std::string(num_hashes, current_fill);
		const auto empty_bar  = std::string(num_spaces, ' ');
		std::print("\r[{}{}] {:.2f}%", filled_bar, empty_bar, percentage);
		std::fflush(stdout);

		if (current_progress == current_total)
		{
			std::print("\n");
		}
	}

	auto reset() -> void { m_progress.store(0); }

	auto done() const -> bool { return m_progress.load() >= m_total.load(); }

	auto increment(std::size_t p_amount = 1) -> void { m_progress.fetch_add(p_amount); }

private:
	std::atomic<std::size_t> m_progress{0};
	std::atomic<std::size_t> m_total{0};
	std::atomic<char> m_fill_char{'#'};
	std::atomic<bool> m_is_incremental{false};
	std::atomic<bool> m_is_verbose{false};

	std::mutex m_print_mutex;
};

class progress_exc : public std::runtime_error
{
public:
	explicit progress_exc(const std::string& p_message) : std::runtime_error("Error: " + p_message) {}
};

#endif	  // PROGRESS_HPP