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

	#include "concepts.hpp"
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

	enum class seek_dir : std::int8_t
	{
		begin	= SEEK_SET,
		current = SEEK_CUR,
		end		= SEEK_END
	};

	enum class open_mode : std::uint8_t
	{
		read	 = 0x01,
		write	 = 0x02,
		truncate = 0x04,
		create	 = 0x08,
		create_write = 0x0A,			// create | write
		create_write_truncate = 0x0E	// create | write | truncate
	};

	inline auto operator|(open_mode p_lhs, open_mode p_rhs) -> open_mode
	{
		return static_cast<open_mode>(std::to_underlying(p_lhs) | std::to_underlying(p_rhs));
	}

	inline auto operator&(open_mode p_lhs, open_mode p_rhs) -> bool
	{
		return (std::to_underlying(p_lhs) & std::to_underlying(p_rhs)) != 0;
	}

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
		open_mode m_mode			   = open_mode::read;

	public:
		~file_view() { close_descriptor(); }

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
		explicit file_view(fs::path p_path, open_mode p_mode = open_mode::read)
			: m_path(std::move(p_path)), m_page_size(static_cast<std::uintmax_t>(::sysconf(_SC_PAGESIZE))), m_mode(p_mode)
		{
			std::int32_t flags = O_CLOEXEC;

			if (m_mode & open_mode::write)
			{
				flags |= O_RDWR;
				if (m_mode & open_mode::create)
				{
					flags |= O_CREAT;
				}
				if (m_mode & open_mode::truncate)
				{
					flags |= O_TRUNC;
				}
			}
			else
			{
				flags |= O_RDONLY;
			}

			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
			m_file_descriptor = ::open(m_path.c_str(), flags, 0644);

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

			if (m_file_size == 0)
			{
				return;
			}
			if (m_file_size <= mem_size::medium)
			{
				file_posix_advise(POSIX_FADV_SEQUENTIAL);
			}
			else if (m_file_size <= mem_size::large)
			{
				file_posix_advise(POSIX_FADV_WILLNEED);
			}

			std::int32_t mmap_flags = (m_mode & open_mode::write) ? MAP_SHARED : MAP_PRIVATE;

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

			const std::int32_t prot = (m_mode & open_mode::write) ? (PROT_READ | PROT_WRITE) : PROT_READ;
			m_map					= ::mmap(nullptr, static_cast<std::size_t>(m_file_size), prot, mmap_flags, m_file_descriptor, 0);

			if (m_map == MAP_FAILED)
			{
				close_descriptor();
				MACRO_THROW(std::runtime_error, std::format("Failed to map file '{}' into memory: {}", m_path.string(), ::strerror(errno)));
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
					MACRO_THROW(
						std::runtime_error(std::format("Failed to apply memory advice: {} for file size: {}", ::strerror(errno), m_file_size)));
				}
			}
		}

		auto file_posix_advise(std::int32_t p_advice) const -> void
		{
			if (m_file_descriptor >= 0 && m_file_size > 0)
			{
				if (::posix_fadvise(m_file_descriptor, 0, static_cast<off_t>(m_file_size), p_advice) > 0)
				{
					MACRO_THROW(
						std::runtime_error(std::format("Failed to apply POSIX advice: {} for file size: {}", ::strerror(errno), m_file_size)));
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

		MACRO_NODISCARD auto is_mapped() const noexcept -> bool { return m_map != MAP_FAILED; }

		static auto mem_available_bytes() -> std::uint64_t
		{
			struct sysinfo info{};
			std::memset(&info, 0, sizeof(info));
			if (::sysinfo(&info) != 0)
			{
				return 0;
			}

			return static_cast<std::uint64_t>(info.freeram) * info.mem_unit;
		}

	public:
		MACRO_NODISCARD auto data() const noexcept -> const std::uint8_t* { return static_cast<const std::uint8_t*>(m_map); }

		MACRO_NODISCARD auto size() const noexcept -> std::uintmax_t { return m_file_size; }

		MACRO_NODISCARD auto is_open() const noexcept -> bool { return m_file_descriptor >= 0; }

		MACRO_NODISCARD auto begin() const noexcept -> const std::uint8_t* { return data(); }

		MACRO_NODISCARD auto end() const noexcept -> const std::uint8_t* { return data() + size(); }

		template <typename range_t> auto write(const range_t& p_range) -> enable_if_t<is_range<range_t>::value, void>
		{
			static_assert(is_range<range_t>::value, "Type must be a range with begin() and end()");

			if (!is_open())
			{
				MACRO_THROW(std::runtime_error(std::format("File '{}' is not open for writing", m_path.string())));
			}

			if (!(m_mode & open_mode::write))
			{
				MACRO_THROW(std::runtime_error(std::format("File '{}' was not opened in write mode", m_path.string())));
			}

			auto src_begin = p_range.begin();
			const auto src_end = p_range.end();
			const std::uintmax_t write_size = std::distance(src_begin, src_end);

			if (m_file_size == 0 || write_size > m_file_size)
			{
				unmap();

				if (::ftruncate(m_file_descriptor, static_cast<off_t>(write_size)) < 0)
				{
					MACRO_THROW(std::runtime_error(std::format("Failed to resize file '{}': {}", m_path.string(), ::strerror(errno))));
				}

				m_file_size = write_size;

				const std::int32_t prot = PROT_READ | PROT_WRITE;
				m_map = ::mmap(nullptr, static_cast<std::size_t>(m_file_size), prot, MAP_SHARED, m_file_descriptor, 0);

				if (m_map == MAP_FAILED)
				{
					MACRO_THROW(std::runtime_error(std::format("Failed to remap file '{}': {}", m_path.string(), ::strerror(errno))));
				}
			}

			auto* dest = static_cast<std::uint8_t*>(m_map);
			std::uintmax_t written = 0;

			while (src_begin != src_end && written < m_file_size)
			{
				dest[written] = static_cast<std::uint8_t>(*src_begin);
				++src_begin;
				++written;
			}
		}

		template <typename range_t> auto read(range_t& p_range) -> enable_if_t<is_range<range_t>::value, void>
		{
			static_assert(is_range<range_t>::value, "Type must be a range with begin() and end()");
			static_assert(has_insert<range_t>::value, "Type must have insert()");

			if (!is_open())
			{
				MACRO_THROW(std::runtime_error(std::format("File '{}' is not open for reading", m_path.string())));
			}

			const auto* src = static_cast<const std::uint8_t*>(m_map);
			for (std::uintmax_t i = 0; i < m_file_size; ++i)
			{
				p_range.insert(p_range.end(), src[i]);
			}
		}
	};
}	 // namespace utils
#endif	  // FILE_VIEW_HPP
