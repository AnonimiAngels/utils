#pragma once
#ifndef PROFILER_HPP
	#define PROFILER_HPP

	#include <algorithm>
	#include <chrono>
	#include <iomanip>
	#include <iostream>
	#include <map>
	#include <memory>
	#include <sstream>
	#include <string>
	#include <utility>
	#include <vector>

namespace utils
{

	class profiler;	   // Forward declaration

	// Created on stack, will have start and end timing
	class profiler_recorder
	{
	public:
		using self_t	 = profiler_recorder;
		using time_point = std::chrono::time_point<std::chrono::steady_clock>;

	private:
		// ~~~ Member variables ~~~
		std::string m_name;
		std::string m_file_name;
		std::int32_t m_line;
		time_point m_start_time;
		profiler* m_profiler;

	public:
		// Allow profiler access to private members
		friend class profiler;

		// ~~~ Public functions ~~~
		~profiler_recorder();
		profiler_recorder() = delete;

		profiler_recorder(std::string p_name, std::string p_file_name, std::int32_t p_line)
			: m_name(std::move(p_name)), m_file_name(std::move(p_file_name)), m_line(p_line), m_start_time(std::chrono::steady_clock::now()), m_profiler(NULL)
		{
		}

		profiler_recorder(const self_t&)		 = delete;
		profiler_recorder(self_t&&)				 = default;
		auto operator=(const self_t&) -> self_t& = delete;
		auto operator=(self_t&&) -> self_t&		 = default;

		auto get_name() const -> const std::string& { return m_name; }

		auto elapsed() const -> std::uint32_t { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - m_start_time).count(); }

	private:
		// ~~~ Private functions ~~~

	public:
		// ~~~ Setters and Getters ~~~
	};

	class profiler
	{
	public:
		struct frame
		{
			std::uint32_t m_count	   = 0;	   // Number of times this function was called
			std::uint32_t m_total_time = 0;	   // Total time spent in this function (in nanoseconds)
			std::uint32_t m_min_time   = 0;	   // Minimum time spent in this function (in nanoseconds)
			std::uint32_t m_max_time   = 0;	   // Maximum time spent in this function (in nanoseconds)
		};

		struct hierarchical_frame
		{
			std::string m_name;
			std::string m_file_name;
			std::int32_t m_line			 = 0;
			std::uint32_t m_depth		 = 0;
			std::uint32_t m_elapsed_time = 0;
			std::vector<std::shared_ptr<hierarchical_frame> > m_children;
			hierarchical_frame* m_parent = NULL;

			hierarchical_frame(const std::string& name, const std::string& file, std::int32_t line, std::uint32_t depth)
				: m_name(name), m_file_name(file), m_line(line), m_depth(depth), m_elapsed_time(0), m_parent(NULL)
			{
			}
		};

	private:
		// ~~~ Member variables ~~~
		std::map<std::string, frame> m_functions;								   // Map of function names to their profiling data
		std::vector<profiler_recorder*> m_call_stack;							   // Active call stack for hierarchy tracking
		std::vector<std::shared_ptr<hierarchical_frame> > m_hierarchical_stack;	   // Stack of active hierarchical frames
		std::vector<std::shared_ptr<hierarchical_frame> > m_call_tree;			   // Root level calls

	public:
		// ~~~ Public functions ~~~
		profiler(const profiler&)					 = delete;
		profiler(profiler&&)						 = default;
		auto operator=(const profiler&) -> profiler& = delete;
		auto operator=(profiler&&) -> profiler&		 = default;

		static auto get_instance(const char* p_name = "default") -> profiler&
		{
			static std::map<std::string, profiler> instances;
			return instances[p_name];
		}

		auto push_recorder(profiler_recorder& p_recorder) -> void;
		auto pop_recorder(profiler_recorder& p_recorder) -> void;

		auto print_summary() const -> void;
		auto print_tree() const -> void;
		auto clear() -> void;

		auto record_timing(const std::string& p_name, std::uint32_t p_elapsed_time) -> void;
		auto record_hierarchical_timing(const profiler_recorder& p_recorder, std::uint32_t p_elapsed_time) -> void;

		~profiler() = default;
		profiler()	= default;

	private:
		// ~~~ Private functions ~~~
		static auto format_time(std::uint32_t p_time_ns) -> std::string;
		static auto format_frame(const std::string& p_name, const frame& p_frame) -> std::string;
		static auto print_hierarchical_frame(const std::shared_ptr<hierarchical_frame>& p_frame, std::uint32_t p_indent = 0) -> void;
		static auto clear_parent_pointers(const std::shared_ptr<hierarchical_frame>& p_frame) -> void;

	public:
		// ~~~ Setters and Getters ~~~
	};

	// ~~~ Method implementations ~~~

	inline profiler_recorder::~profiler_recorder()
	{
		if (m_profiler != NULL)
		{
			std::uint32_t elapsed_time = elapsed();
			m_profiler->record_timing(m_name, elapsed_time);
			m_profiler->record_hierarchical_timing(*this, elapsed_time);
			m_profiler->pop_recorder(*this);
		}
	}

	inline auto profiler::push_recorder(profiler_recorder& p_recorder) -> void
	{
		p_recorder.m_profiler = this;
		m_call_stack.push_back(&p_recorder);

		// Create hierarchical frame and add to appropriate location
		std::uint32_t depth = m_hierarchical_stack.size();
		auto new_frame		= std::make_shared<hierarchical_frame>(p_recorder.m_name, p_recorder.m_file_name, p_recorder.m_line, depth);

		if (depth == 0)
		{
			// Root level call
			m_call_tree.push_back(new_frame);
			m_hierarchical_stack.push_back(new_frame);
		}
		else
		{
			// Child call - add to current parent
			auto parent			= m_hierarchical_stack.back();
			new_frame->m_parent = parent.get();
			parent->m_children.push_back(new_frame);
			m_hierarchical_stack.push_back(new_frame);
		}
	}

	inline auto profiler::pop_recorder(profiler_recorder& p_recorder) -> void
	{
		if (!m_call_stack.empty() && m_call_stack.back() == &p_recorder)
		{
			m_call_stack.pop_back();
		}
	}

	inline auto profiler::record_timing(const std::string& p_name, std::uint32_t p_elapsed_time) -> void
	{
		frame& f = m_functions[p_name];

		if (f.m_count == 0)
		{
			f.m_min_time = p_elapsed_time;
			f.m_max_time = p_elapsed_time;
		}
		else
		{
			f.m_min_time = std::min(p_elapsed_time, f.m_min_time);
			f.m_max_time = std::max(p_elapsed_time, f.m_max_time);
		}

		f.m_count++;
		f.m_total_time += p_elapsed_time;
	}

	inline auto profiler::record_hierarchical_timing(const profiler_recorder&, std::uint32_t p_elapsed_time) -> void
	{
		// Pop the corresponding frame from hierarchical stack
		if (!m_hierarchical_stack.empty())
		{
			auto current_frame = m_hierarchical_stack.back();
			m_hierarchical_stack.pop_back();

			// Set the elapsed time for this frame
			current_frame->m_elapsed_time = p_elapsed_time;
		}
	}

	inline auto profiler::format_frame(const std::string& p_name, const frame& p_frame) -> std::string
	{
		std::stringstream ss;
		std::uint32_t avg_time = (p_frame.m_count > 0) ? (p_frame.m_total_time / p_frame.m_count) : 0;

		// Truncate long function names
		std::string display_name	 = p_name;
		const size_t max_name_length = 80;
		if (display_name.length() > max_name_length)
		{
			display_name = display_name.substr(0, max_name_length - 3) + "...";
		}

		ss << std::left << std::setw(83) << display_name << std::right << std::setw(8) << p_frame.m_count << std::setw(14) << format_time(p_frame.m_total_time) << std::setw(14)
		   << format_time(avg_time) << std::setw(14) << format_time(p_frame.m_min_time) << std::setw(14) << format_time(p_frame.m_max_time);

		return ss.str();
	}

	inline auto profiler::print_summary() const -> void
	{
		std::cout << "\n=== Profiler Summary ===\n";
		std::cout << std::left << std::setw(83) << "Function Name" << std::right << std::setw(8) << "Calls" << std::setw(14) << "Total" << std::setw(14) << "Avg" << std::setw(14) << "Min"
				  << std::setw(14) << "Max" << "\n";
		std::cout << std::string(147, '-') << "\n";

		for (const auto& m_function : m_functions)
		{
			std::cout << format_frame(m_function.first, m_function.second) << "\n";
		}
		std::cout << "\n";
	}

	inline auto profiler::print_tree() const -> void
	{
		std::cout << "\n=== Profiler Call Tree ===\n";
		for (const auto& root_frame : m_call_tree)
		{
			print_hierarchical_frame(root_frame, 0);
		}
		std::cout << "\n";
	}

	inline auto profiler::clear() -> void
	{
		// Clear parent pointers first to prevent dangling pointer issues
		for (const auto& root_frame : m_call_tree)
		{
			clear_parent_pointers(root_frame);
		}

		m_functions.clear();
		m_call_stack.clear();
		m_hierarchical_stack.clear();
		m_call_tree.clear();
	}

	inline auto profiler::print_hierarchical_frame(const std::shared_ptr<hierarchical_frame>& p_frame, std::uint32_t p_indent) -> void
	{
		// Create indentation
		std::string indent(p_indent * 2, ' ');

		const auto last_slash = p_frame->m_file_name.find_last_of("/\\");
		const auto& file_name = (last_slash != std::string::npos) ? p_frame->m_file_name.substr(last_slash + 1) : p_frame->m_file_name;
		std::cout << indent << "├─ " << p_frame->m_name << " (" << file_name << ":" << p_frame->m_line << ") "
				  << "[" << format_time(p_frame->m_elapsed_time) << "]\n";

		// Print children
		for (const auto& child : p_frame->m_children)
		{
			print_hierarchical_frame(child, p_indent + 1);
		}
	}

	inline auto profiler::format_time(std::uint32_t p_time_ns) -> std::string
	{
		std::stringstream ss;
		if (p_time_ns >= 1000000)
		{
			double time_ms = static_cast<double>(p_time_ns) / 1000000.0;
			ss << std::fixed << std::setprecision(2) << time_ms << "ms";
		}
		else
		{
			ss << p_time_ns << "ns";
		}
		return ss.str();
	}

	inline auto profiler::clear_parent_pointers(const std::shared_ptr<hierarchical_frame>& p_frame) -> void
	{
		if (p_frame)
		{
			p_frame->m_parent = NULL;
			for (const auto& child : p_frame->m_children)
			{
				clear_parent_pointers(child);
			}
		}
	}

}	 // namespace utils
#endif	  // PROFILER_HPP
