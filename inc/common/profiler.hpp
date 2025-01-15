#pragma once
#ifndef PROFILER_HPP
#define PROFILER_HPP

#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <filesystem>

#include "common/loggers.hpp"

namespace utils
{
	using fpoint_us = std::chrono::duration<double, std::micro>;

	struct profile_result
	{
		profile_result(const std::string_view& in_name, const fpoint_us& in_start, const std::chrono::microseconds& in_elapsed_time, const std::thread::id& in_thread_id)
			: m_name(in_name), m_start(in_start), m_elapsed_time(in_elapsed_time), m_thread_id(in_thread_id)
		{
		}

		std::string_view m_name;
		fpoint_us m_start;
		std::chrono::microseconds m_elapsed_time;
		std::thread::id m_thread_id;
	};

	struct instrument_session
	{
		instrument_session(const std::string_view& in_name) : m_name(in_name) {}
		std::string m_name;
	};

	class profiler
	{
	  private:
		std::mutex m_mutex;
		std::unique_ptr<instrument_session> m_session = nullptr;
		std::ofstream m_out_stream;

	  public:
		profiler(const profiler&)					 = delete;
		profiler(profiler&&)						 = delete;
		auto operator=(const profiler&) -> profiler& = delete;
		auto operator=(profiler&&) -> profiler&		 = delete;

		void begin_session(const std::string_view& in_name, const std::string_view& in_filepath)
		{
			std::lock_guard lock(m_mutex);
			if (m_session != nullptr)
			{
				utils::logger(__FUNCTION__).error("profiler::begin_session('{0}') when session '{1}' already open.", in_name, m_session->m_name);
				internal_end_session();
			}

			// Check if base directory exists
			const auto l_base_dir = std::filesystem::path(in_filepath).parent_path();
			if (!std::filesystem::exists(l_base_dir)) { std::filesystem::create_directories(l_base_dir); }

			m_out_stream.open(in_filepath.cbegin());

			if (m_out_stream.is_open())
			{
				m_session = std::make_unique<instrument_session>(in_name);
				write_header();
			}
			else { utils::logger(__FUNCTION__).error("profiler could not open out file '{}'.", in_filepath); }
		}

		void end_session()
		{
			std::lock_guard lock(m_mutex);
			internal_end_session();
		}

		void write_profile(const profile_result& in_result)
		{
			std::lock_guard lock(m_mutex);
			if (m_session != nullptr)
			{
				const auto& l_result = std::format(R"(,{{"cat":"function","dur":{},"name":"{}","ph":"X","pid":0,"tid":{},"ts":{:0.3f}}})", in_result.m_elapsed_time.count(),
												   in_result.m_name, in_result.m_thread_id, in_result.m_start.count());

				m_out_stream << l_result;
				m_out_stream.flush();
			}
		}

		static auto get() -> profiler&
		{
			static profiler instance;
			return instance;
		}

	  private:
		profiler() = default;
		~profiler() { end_session(); }

		void write_header()
		{
			m_out_stream << R"({"otherData": {},"traceEvents":[{})";
			m_out_stream.flush();
		}

		void write_footer()
		{
			m_out_stream << "]}";
			m_out_stream.flush();
		}

		void internal_end_session()
		{
			if (m_session == nullptr) { return; }

			write_footer();
			m_out_stream.close();
			m_session.reset();
		}
	};

	class profiler_timer
	{
	  public:
		~profiler_timer()
		{
			if (!m_stopped) { stop(); }
		}

		profiler_timer(const std::string_view& in_name) : m_name(in_name) { m_start = std::chrono::steady_clock::now(); }
		profiler_timer(const profiler_timer&)					 = delete;
		profiler_timer(profiler_timer&&)						 = delete;
		auto operator=(const profiler_timer&) -> profiler_timer& = delete;
		auto operator=(profiler_timer&&) -> profiler_timer&		 = delete;

		void stop()
		{
			auto l_end_point		   = std::chrono::steady_clock::now();
			fpoint_us l_high_res_start = m_start.time_since_epoch();
			auto l_elapsed			   = std::chrono::time_point_cast<std::chrono::microseconds>(l_end_point).time_since_epoch()
						   - std::chrono::time_point_cast<std::chrono::microseconds>(m_start).time_since_epoch();

			profiler::get().write_profile({m_name, l_high_res_start, l_elapsed, std::this_thread::get_id()});

			m_stopped = true;
		}

	  private:
		std::string_view m_name;
		std::chrono::time_point<std::chrono::steady_clock> m_start;
		bool m_stopped = false;
	};

	namespace profiler_utils
	{
		template <std::size_t in_size> struct change_result
		{
			char m_data[in_size] = {};
		};

		template <std::size_t in_size_lhs, std::size_t in_size_rhs>
		constexpr auto cleanup_output_string(const char (&in_expr)[in_size_lhs], const char (&in_remove)[in_size_rhs]) -> change_result<in_size_lhs>
		{
			change_result<in_size_lhs> result = {};

			std::size_t l_src_index = 0;
			std::size_t l_dst_index = 0;
			while (l_src_index < in_size_lhs)
			{
				std::size_t l_match_index = 0;
				while (l_match_index < in_size_rhs - 1 && l_src_index + l_match_index < in_size_lhs - 1 && in_expr[l_src_index + l_match_index] == in_remove[l_match_index])
				{
					++l_match_index;
				}

				if (l_match_index == in_size_rhs - 1) { l_src_index += l_match_index; }
				result.m_data[l_dst_index++] = in_expr[l_src_index] == '"' ? '\'' : in_expr[l_src_index];
				++l_src_index;
			}

			return result;
		}

	} // namespace profiler_utils
} // namespace utils

#define MACRO_PROFILE 1

#if MACRO_PROFILE
  // Resolve which function signature macro will be used. Note that this only
// is resolved when the (pre)compiler starts, so the syntax highlighting
// could mark the wrong one in your editor!
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define MACRO_FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define MACRO_FUNC_SIG __PRETTY_FUNCTION__
#elif (defined(__FUNCSIG__) || (_MSC_VER))
#define MACRO_FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define MACRO_FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define MACRO_FUNC_SIG __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define MACRO_FUNC_SIG __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define MACRO_FUNC_SIG __func__
#else
#define MACRO_FUNC_SIG "MACRO_FUNC_SIG unknown!"
#endif

#define MACRO_PROFILE_BEGIN_SESSION(name, filepath) ::utils::profiler::get().begin_session(name, filepath)
#define MACRO_PROFILE_END_SESSION() ::utils::profiler::get().end_session()
#define MACRO_PROFILE_SCOPE_LINE2(name, line)                                                              \
	constexpr auto cl_fixed_name##line = ::utils::profiler_utils::cleanup_output_string(name, "__cdecl "); \
	::utils::profiler_timer timer##line(cl_fixed_name##line.m_data)
#define MACRO_PROFILE_SCOPE_LINE(name, line) MACRO_PROFILE_SCOPE_LINE2(name, line)
#define MACRO_PROFILE_SCOPE(name) MACRO_PROFILE_SCOPE_LINE(name, __LINE__)
#define MACRO_PROFILE_FUNCTION() MACRO_PROFILE_SCOPE(MACRO_FUNC_SIG)
#else
#define MACRO_PROFILE_BEGIN_SESSION(name, filepath)
#define MACRO_PROFILE_END_SESSION()
#define MACRO_PROFILE_SCOPE(name)
#define MACRO_PROFILE_FUNCTION()
#endif

#endif // PROFILER_HPP
