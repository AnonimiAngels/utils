#pragma once
#ifndef FILE_VIEW_HPP
	#define FILE_VIEW_HPP

	#include <cstdint>
	#include <utility>

	#include <fcntl.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <sys/sysinfo.h>
	#include <unistd.h>

	#include "filesystem.hpp"
	#include "format.hpp"
	#include "utils_macros.hpp"

namespace utils
{
	namespace mem_size
	{
		enum : std::uintmax_t
		{
			tiny	= 65536,		 // 64 KiB
			small	= 1048576,		 // 1 MiB
			medium	= 16777216,		 // 16 MiB
			large	= 268435456,	 // 256 MiB
			huge	= 1073741824,	 // 1 GiB
			massive = 10737418240	 // 10 GiB
		};
	}

	enum class file_mode : std::uint8_t
	{
		read	 = 0x01,
		write	 = 0x02,
		append	 = 0x04,
		binary	 = 0x08,
		truncate = 0x10
	};

	inline auto operator|(file_mode p_lhs, file_mode p_rhs) -> file_mode
	{
		return static_cast<file_mode>(static_cast<std::uint8_t>(p_lhs) | static_cast<std::uint8_t>(p_rhs));
	}

	inline auto operator&(file_mode p_lhs, file_mode p_rhs) -> bool
	{
		return (static_cast<std::uint8_t>(p_lhs) & static_cast<std::uint8_t>(p_rhs)) != 0;
	}

	enum class seek_dir : std::int32_t
	{
		begin	= SEEK_SET,
		current = SEEK_CUR,
		end		= SEEK_END
	};

	enum class file_error : std::uint8_t
	{
		none = 0,
		not_open,
		already_open,
		open_failed,
		read_failed,
		write_failed,
		seek_failed,
		tell_failed,
		flush_failed,
		close_failed,
		invalid_mode,
		eof_reached
	};

	class file_view
	{
	public:
		using self_t = file_view;

	private:
		fs::path m_path;

		std::int32_t m_file_descriptor = -1;
		void* m_map					   = MAP_FAILED;
		std::uintmax_t m_file_size	   = 0;
		std::uintmax_t m_page_size	   = 0;
		std::uintmax_t m_prefetch_size = 0;

	public:
		~file_view() { close_descriptor(); }

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
		explicit file_view(fs::path p_path)
			: m_path(std::move(p_path)),
			  m_file_descriptor(::open(m_path.c_str(), O_RDONLY | O_CLOEXEC)),
			  m_page_size(static_cast<std::uintmax_t>(::sysconf(_SC_PAGESIZE)))
		{

			if (m_file_descriptor < 0)
			{
				MACRO_THROW(std::runtime_error(std::format("Failed to open file '{}': {}", m_path.string(), ::strerror(errno))));
			}

			struct stat file_stat = {};
			std::memset(&file_stat, 0, sizeof(file_stat));

			if (::fstat(m_file_descriptor, &file_stat) < 0)
			{
				close_descriptor();
				MACRO_THROW(std::runtime_error(std::format("Failed to get file status for '{}': {}", m_path.string(), ::strerror(errno))));
			}

			m_file_size = static_cast<std::size_t>(file_stat.st_size);

			// Optimize kernel read-ahead based on file size
			if (m_file_size <= mem_size::medium)
			{
				file_posix_advise(POSIX_FADV_SEQUENTIAL);
			}
			else if (m_file_size <= mem_size::large)
			{
				file_posix_advise(POSIX_FADV_WILLNEED);
			}

			std::int32_t mmap_flags = MAP_PRIVATE;

			if (m_file_size <= mem_size::tiny)
			{
				mmap_flags |= MAP_POPULATE;
			}

			constexpr std::uintmax_t min_mem_size	   = 4ULL * 1024ULL * 1024ULL;		// 4 MiB
			constexpr std::uintmax_t min_mem_size_huge = 256ULL * 1024ULL * 1024ULL;	// 256 MiB

			if (m_file_size <= std::min(mem_available_bytes() / 4, min_mem_size))
			{
				mmap_flags |= MAP_POPULATE;
			}

			if (m_file_size >= min_mem_size_huge)
			{
				mmap_flags |= MAP_HUGETLB;
			}

			m_map = ::mmap(nullptr, static_cast<std::size_t>(m_file_size), PROT_READ, mmap_flags, m_file_descriptor, 0);

			if (m_map == MAP_FAILED)
			{
				close_descriptor();
				MACRO_THROW(std::runtime_error(std::format("Failed to map file '{}' into memory: {}", m_path.string(), ::strerror(errno))));
			}

			apply_memory_advice();
		}

		file_view(const self_t&)				 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		file_view(self_t&& p_other) noexcept
			: m_file_descriptor(p_other.m_file_descriptor), m_map(p_other.m_map), m_file_size(p_other.m_file_size), m_page_size(p_other.m_page_size)
		{
			p_other.m_file_descriptor = -1;
			p_other.m_map			  = MAP_FAILED;
			p_other.m_file_size		  = 0;
			p_other.m_page_size		  = 0;
		}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				close_descriptor();

				m_file_descriptor = p_other.m_file_descriptor;
				m_map			  = p_other.m_map;
				m_file_size		  = p_other.m_file_size;
				m_page_size		  = p_other.m_page_size;

				p_other.m_file_descriptor = -1;
				p_other.m_map			  = MAP_FAILED;
				p_other.m_file_size		  = 0;
				p_other.m_page_size		  = 0;
			}
			return *this;
		}

	private:
		auto apply_memory_advice() -> void
		{
			if (m_file_size <= mem_size::tiny)
			{
				return;
			}

			if (m_file_size <= mem_size::small)
			{
				file_advise(MADV_SEQUENTIAL);
				file_advise(MADV_WILLNEED);
			}
			// Medium files: Sequential access pattern
			else if (m_file_size <= mem_size::medium)
			{
				file_advise(MADV_SEQUENTIAL);
			}
			else if (m_file_size <= mem_size::large)
			{
				file_advise(MADV_NORMAL);
				m_prefetch_size = std::min<decltype(m_file_size)>(m_file_size / 4, mem_size::medium);
				file_advise(MADV_WILLNEED);
			}
			else if (m_file_size <= mem_size::huge)
			{
				file_advise(MADV_RANDOM);
			}
			else
			{
				file_advise(MADV_RANDOM);
	#ifdef MADV_HUGEPAGE
				file_advise(MADV_HUGEPAGE);
	#endif
			}
		}

		auto file_advise(std::int32_t p_advice) -> void
		{
			if (is_mapped() && m_file_size > 0)
			{
				const auto file_len = m_file_size == m_prefetch_size ? m_file_size : m_prefetch_size;

				if (::madvise(m_map, static_cast<std::size_t>(file_len), p_advice) < 0)
				{
					MACRO_THROW(std::runtime_error(std::format("Failed to apply memory advice: {} for file size: {}", ::strerror(errno), m_file_size)));
				}
			}
		}

		auto file_posix_advise(std::int32_t p_advice) const -> void
		{
			if (m_file_descriptor >= 0 && m_file_size > 0)
			{
				if (::posix_fadvise(m_file_descriptor, 0, static_cast<off_t>(m_file_size), p_advice) > 0)
				{
					MACRO_THROW(std::runtime_error(std::format("Failed to apply POSIX advice: {} for file size: {}", ::strerror(errno), m_file_size)));
				}
			}
		}

		auto unmap() noexcept -> void
		{
			if (is_mapped())
			{
				::munmap(m_map, static_cast<std::size_t>(m_file_size));
				m_map = MAP_FAILED;
			}
		}

		auto close_descriptor() noexcept -> void
		{
			unmap();

			if (m_file_descriptor >= 0)
			{
				::close(m_file_descriptor);
				m_file_descriptor = -1;
			}
		}

		auto is_mapped() const noexcept -> bool { return m_map != MAP_FAILED; }

		static auto mem_available_bytes() -> std::uint64_t
		{
			struct sysinfo info;
			std::memset(&info, 0, sizeof(info));
			if (::sysinfo(&info) != 0)
			{
				return 0;
			}

			return static_cast<std::uint64_t>(info.freeram) * info.mem_unit;
		}

	public:
		auto data() const noexcept -> const std::uint8_t* { return static_cast<const std::uint8_t*>(m_map); }

		auto size() const noexcept -> std::uintmax_t { return m_file_size; }
	};
}	 // namespace utils
#endif	  // FILE_VIEW_HPP
