#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include "cmemory.hpp"
#include "concepts.hpp"
#include "debug.hpp"
#include "expected.hpp"
#include "filesystem.hpp"
#include "format.hpp"
#include "ranges.hpp"

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

	enum class memory_advice : std::int32_t
	{
		normal	   = MADV_NORMAL,
		random	   = MADV_RANDOM,
		sequential = MADV_SEQUENTIAL,
		willneed   = MADV_WILLNEED,
		dontneed   = MADV_DONTNEED,
		hugepage   = MADV_HUGEPAGE,
		nohugepage = MADV_NOHUGEPAGE
	};

	enum class file_advice : std::int32_t
	{
		normal	   = POSIX_FADV_NORMAL,
		random	   = POSIX_FADV_RANDOM,
		sequential = POSIX_FADV_SEQUENTIAL,
		willneed   = POSIX_FADV_WILLNEED,
		dontneed   = POSIX_FADV_DONTNEED,
		noreuse	   = POSIX_FADV_NOREUSE
	};

	inline auto mem_available_bytes() -> std::size_t
	{
		struct sysinfo info;
		if (sysinfo(&info) == 0)
		{
			return info.freeram * info.mem_unit;
		}
		return 0;
	}

	inline auto apply_memory_advice(void* p_addr, std::size_t p_length, memory_advice p_advice) -> bool
	{
		return madvise(p_addr, p_length, static_cast<std::int32_t>(p_advice)) == 0;
	}

	inline auto file_posix_advise(std::int32_t p_fd, std::int64_t p_offset, std::int64_t p_len, file_advice p_advice) -> bool
	{
		return posix_fadvise(p_fd, p_offset, p_len, static_cast<std::int32_t>(p_advice)) == 0;
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
		eof_reached,
		mmap_failed,
		stat_failed,
		memory_alloc_failed
	};

	inline auto error_to_string(file_error p_error) -> const char*
	{
		switch (p_error)
		{
		case file_error::none:
			return "No error";
		case file_error::not_open:
			return "File not open";
		case file_error::already_open:
			return "File already open";
		case file_error::open_failed:
			return "Failed to open file";
		case file_error::read_failed:
			return "Read operation failed";
		case file_error::write_failed:
			return "Write operation failed";
		case file_error::seek_failed:
			return "Seek operation failed";
		case file_error::tell_failed:
			return "Tell operation failed";
		case file_error::flush_failed:
			return "Flush operation failed";
		case file_error::close_failed:
			return "Close operation failed";
		case file_error::invalid_mode:
			return "Invalid file mode";
		case file_error::eof_reached:
			return "End of file reached";
		case file_error::mmap_failed:
			return "Memory mapping failed";
		case file_error::stat_failed:
			return "File stat failed";
		case file_error::memory_alloc_failed:
			return "Memory allocation failed";
		default:
			return "Unknown error";
		}
	}

	class file
	{
	public:
		using self_t	= file;
		using error_t	= file_error;
		using mode_t	= file_mode;
		using seek_t	= seek_dir;
		using size_type = std::size_t;
		using pos_type	= std::int64_t;
		using byte_t	= std::uint8_t;

	private:
		std::int32_t m_fd;
		fs::path m_path;
		mode_t m_mode;
		byte_t* m_data;
		size_type m_size;
		size_type m_capacity;
		pos_type m_position;
		bool m_is_open;
		bool m_is_mmap;
		bool m_is_ram;

	public:
		~file() { cleanup(); }

		file()
			: m_fd(-1),
			  m_path(),
			  m_mode(mode_t::read),
			  m_data(nullptr),
			  m_size(0),
			  m_capacity(0),
			  m_position(0),
			  m_is_open(false),
			  m_is_mmap(false),
			  m_is_ram(true)
		{
			UTILS_DEBUG_LOG("file::file() - Default constructor\n");
		}

		explicit file(const fs::path& p_path, mode_t p_mode = mode_t::read, bool p_load_ram = true)
			: m_fd(-1),
			  m_path(p_path),
			  m_mode(p_mode),
			  m_data(nullptr),
			  m_size(0),
			  m_capacity(0),
			  m_position(0),
			  m_is_open(false),
			  m_is_mmap(false),
			  m_is_ram(p_load_ram)
		{
			UTILS_DEBUG_LOG("file::file() - Path constructor: {}\n", p_path.string());
			open(p_path, p_mode, p_load_ram);
		}

		file(const self_t&)						 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		file(self_t&& p_other) noexcept
			: m_fd(p_other.m_fd),
			  m_path(std::move(p_other.m_path)),
			  m_mode(p_other.m_mode),
			  m_data(p_other.m_data),
			  m_size(p_other.m_size),
			  m_capacity(p_other.m_capacity),
			  m_position(p_other.m_position),
			  m_is_open(p_other.m_is_open),
			  m_is_mmap(p_other.m_is_mmap),
			  m_is_ram(p_other.m_is_ram)
		{
			p_other.m_fd	   = -1;
			p_other.m_data	   = nullptr;
			p_other.m_size	   = 0;
			p_other.m_capacity = 0;
			p_other.m_position = 0;
			p_other.m_is_open  = false;
			p_other.m_is_mmap  = false;
			UTILS_DEBUG_LOG("file::file() - Move constructor\n");
		}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				cleanup();
				m_fd	   = p_other.m_fd;
				m_path	   = std::move(p_other.m_path);
				m_mode	   = p_other.m_mode;
				m_data	   = p_other.m_data;
				m_size	   = p_other.m_size;
				m_capacity = p_other.m_capacity;
				m_position = p_other.m_position;
				m_is_open  = p_other.m_is_open;
				m_is_mmap  = p_other.m_is_mmap;
				m_is_ram   = p_other.m_is_ram;

				p_other.m_fd	   = -1;
				p_other.m_data	   = nullptr;
				p_other.m_size	   = 0;
				p_other.m_capacity = 0;
				p_other.m_position = 0;
				p_other.m_is_open  = false;
				p_other.m_is_mmap  = false;
			}
			UTILS_DEBUG_LOG("file::operator=() - Move assignment\n");
			return *this;
		}

	private:
		auto cleanup() -> void
		{
			if (m_is_mmap && m_data != nullptr)
			{
				::munmap(m_data, m_size);
			}
			else if (m_is_ram && m_data != nullptr)
			{
				delete[] m_data;
			}
			if (m_fd >= 0)
			{
				::close(m_fd);
			}
			m_data	  = nullptr;
			m_fd	  = -1;
			m_is_open = false;
		}

		auto posix_flags() const -> std::int32_t
		{
			std::int32_t flags = 0;

			if ((m_mode & mode_t::read) && (m_mode & mode_t::write))
			{
				flags = O_RDWR;
			}
			else if (m_mode & mode_t::write)
			{
				flags = O_WRONLY | O_CREAT;
			}
			else
			{
				flags = O_RDONLY;
			}

			if (m_mode & mode_t::append)
			{
				flags |= O_APPEND;
			}
			if (m_mode & mode_t::truncate)
			{
				flags |= O_TRUNC;
			}

			return flags;
		}

	public:
		auto open(const fs::path& p_path, mode_t p_mode = mode_t::read, bool p_load_ram = true) -> expected<void, error_t>
		{
			if (m_is_open)
			{
				UTILS_DEBUG_LOG("file::open() - Already open\n");
				return unexpected<error_t>(error_t::already_open);
			}

			m_path	   = p_path;
			m_mode	   = p_mode;
			m_is_ram   = p_load_ram;
			m_position = 0;

			m_fd = ::open(p_path.string().c_str(), posix_flags(), 0644);
			if (m_fd < 0)
			{
				UTILS_DEBUG_LOG("file::open() - Failed to open: {}\n", p_path.string());
				return unexpected<error_t>(error_t::open_failed);
			}

			struct stat file_stat;
			if (::fstat(m_fd, &file_stat) < 0)
			{
				::close(m_fd);
				m_fd = -1;
				UTILS_DEBUG_LOG("file::open() - Failed to stat: {}\n", p_path.string());
				return unexpected<error_t>(error_t::stat_failed);
			}

			m_size	  = static_cast<size_type>(file_stat.st_size);
			m_is_open = true;

			if (p_load_ram)
			{
				// Check if file size is reasonable for memory
				auto available = mem_available_bytes();
				if (m_size > available / 2)
				{
					UTILS_DEBUG_LOG("file::open() - File too large for RAM ({} > {}), using direct I/O\n", m_size, available / 2);
					m_is_ram = false;
					return {};
				}

				if (m_mode & mode_t::read && !(m_mode & mode_t::write))
				{
					// Try memory mapping for read-only
					m_data = static_cast<byte_t*>(::mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0));
					if (m_data != MAP_FAILED)
					{
						m_is_mmap  = true;
						m_capacity = m_size;

						// Apply appropriate memory advice based on file size
						if (m_size > mem_size::large)
						{
							apply_memory_advice(m_data, m_size, memory_advice::sequential);
						}
						else if (m_size > mem_size::medium)
						{
							apply_memory_advice(m_data, m_size, memory_advice::willneed);
						}

						UTILS_DEBUG_LOG("file::open() - Memory mapped: {} ({} bytes)\n", p_path.string(), m_size);
					}
					else
					{
						m_data = nullptr;
					}
				}

				if (m_data == nullptr && m_size > 0)
				{
					// Load into RAM
					m_data = new byte_t[m_size];
					if (m_data == nullptr)
					{
						::close(m_fd);
						m_fd	  = -1;
						m_is_open = false;
						return unexpected<error_t>(error_t::memory_alloc_failed);
					}

					m_capacity = m_size;

					// Apply file advice for better prefetching
					if (m_size > mem_size::medium)
					{
						file_posix_advise(m_fd, 0, m_size, file_advice::sequential);
					}

					ssize_t bytes_read = ::read(m_fd, m_data, m_size);
					if (bytes_read < 0 || static_cast<size_type>(bytes_read) != m_size)
					{
						delete[] m_data;
						m_data = nullptr;
						::close(m_fd);
						m_fd	  = -1;
						m_is_open = false;
						return unexpected<error_t>(error_t::read_failed);
					}
					UTILS_DEBUG_LOG("file::open() - Loaded to RAM: {} ({} bytes)\n", p_path.string(), m_size);
				}
			}

			UTILS_DEBUG_LOG("file::open() - Opened: {}\n", p_path.string());
			return {};
		}

		auto close() -> expected<void, error_t>
		{
			if (!m_is_open)
			{
				UTILS_DEBUG_LOG("file::close() - Not open\n");
				return unexpected<error_t>(error_t::not_open);
			}

			if (m_is_ram && (m_mode & mode_t::write) && m_data != nullptr && !m_is_mmap)
			{
				// Write back to file if modified
				::lseek(m_fd, 0, SEEK_SET);
				ssize_t bytes_written = ::write(m_fd, m_data, m_size);
				if (bytes_written < 0 || static_cast<size_type>(bytes_written) != m_size)
				{
					UTILS_DEBUG_LOG("file::close() - Failed to write back\n");
				}
			}

			cleanup();
			UTILS_DEBUG_LOG("file::close() - Closed successfully\n");
			return {};
		}

		auto read(void* p_buffer, size_type p_size) -> expected<size_type, error_t>
		{
			if (!m_is_open)
			{
				return unexpected<error_t>(error_t::not_open);
			}

			size_type available = m_size - m_position;
			size_type to_read	= std::min(p_size, available);

			if (to_read == 0)
			{
				return unexpected<error_t>(error_t::eof_reached);
			}

			if (m_data != nullptr)
			{
				std::memcpy(p_buffer, m_data + m_position, to_read);
			}
			else
			{
				::lseek(m_fd, m_position, SEEK_SET);
				ssize_t bytes_read = ::read(m_fd, p_buffer, to_read);
				if (bytes_read < 0)
				{
					return unexpected<error_t>(error_t::read_failed);
				}
				to_read = static_cast<size_type>(bytes_read);
			}

			m_position += to_read;
			UTILS_DEBUG_LOG("file::read() - Read {} bytes\n", to_read);
			return to_read;
		}

		auto write(const void* p_buffer, size_type p_size) -> expected<size_type, error_t>
		{
			if (!m_is_open || !(m_mode & mode_t::write))
			{
				return unexpected<error_t>(error_t::not_open);
			}

			if (m_is_mmap)
			{
				return unexpected<error_t>(error_t::write_failed);
			}

			if (m_is_ram && m_data != nullptr)
			{
				size_type new_size = m_position + p_size;
				if (new_size > m_capacity)
				{
					size_type new_capacity = new_size * 2;
					byte_t* new_data	   = new byte_t[new_capacity];
					if (new_data == nullptr)
					{
						return unexpected<error_t>(error_t::memory_alloc_failed);
					}
					if (m_size > 0)
					{
						std::memcpy(new_data, m_data, m_size);
					}
					delete[] m_data;
					m_data	   = new_data;
					m_capacity = new_capacity;
				}
				std::memcpy(m_data + m_position, p_buffer, p_size);
				m_position += p_size;
				if (m_position > static_cast<pos_type>(m_size))
				{
					m_size = m_position;
				}
			}
			else
			{
				::lseek(m_fd, m_position, SEEK_SET);
				ssize_t bytes_written = ::write(m_fd, p_buffer, p_size);
				if (bytes_written < 0)
				{
					return unexpected<error_t>(error_t::write_failed);
				}
				m_position += bytes_written;
				if (m_position > static_cast<pos_type>(m_size))
				{
					m_size = m_position;
				}
				return static_cast<size_type>(bytes_written);
			}

			UTILS_DEBUG_LOG("file::write() - Wrote {} bytes\n", p_size);
			return p_size;
		}

		auto write(const std::string& p_data) -> expected<size_type, error_t> { return write(p_data.data(), p_data.size()); }

		auto data() const -> const byte_t* { return m_data; }

		auto data() -> byte_t* { return (m_mode & mode_t::write) ? m_data : nullptr; }

		auto size() const -> size_type { return m_size; }

		auto capacity() const -> size_type { return m_capacity; }

		auto seek(pos_type p_offset, seek_t p_origin = seek_t::begin) -> expected<void, error_t>
		{
			if (!m_is_open)
			{
				return unexpected<error_t>(error_t::not_open);
			}

			pos_type new_pos = 0;
			switch (p_origin)
			{
			case seek_t::begin:
				new_pos = p_offset;
				break;
			case seek_t::current:
				new_pos = m_position + p_offset;
				break;
			case seek_t::end:
				new_pos = static_cast<pos_type>(m_size) + p_offset;
				break;
			}

			if (new_pos < 0 || new_pos > static_cast<pos_type>(m_size))
			{
				return unexpected<error_t>(error_t::seek_failed);
			}

			m_position = new_pos;
			UTILS_DEBUG_LOG("file::seek() - Seeked to position: {}\n", m_position);
			return {};
		}

		auto tell() const -> expected<pos_type, error_t>
		{
			if (!m_is_open)
			{
				return unexpected<error_t>(error_t::not_open);
			}
			return m_position;
		}

		auto flush() -> expected<void, error_t>
		{
			if (!m_is_open)
			{
				return unexpected<error_t>(error_t::not_open);
			}

			if (m_is_ram && (m_mode & mode_t::write) && m_data != nullptr && !m_is_mmap)
			{
				::lseek(m_fd, 0, SEEK_SET);
				ssize_t bytes_written = ::write(m_fd, m_data, m_size);
				if (bytes_written < 0 || static_cast<size_type>(bytes_written) != m_size)
				{
					return unexpected<error_t>(error_t::flush_failed);
				}
			}

			if (::fsync(m_fd) != 0)
			{
				return unexpected<error_t>(error_t::flush_failed);
			}

			UTILS_DEBUG_LOG("file::flush() - Flushed successfully\n");
			return {};
		}

		auto file_advise(file_advice p_advice, pos_type p_offset = 0, pos_type p_length = 0) -> expected<void, error_t>
		{
			if (!m_is_open || m_fd < 0)
			{
				return unexpected<error_t>(error_t::not_open);
			}

			pos_type length = (p_length == 0) ? static_cast<pos_type>(m_size) : p_length;
			if (!file_posix_advise(m_fd, p_offset, length, p_advice))
			{
				UTILS_DEBUG_LOG("file::file_advise() - Failed to apply advice\n");
				return unexpected<error_t>(error_t::seek_failed);
			}

			UTILS_DEBUG_LOG("file::file_advise() - Applied advice: {}\n", static_cast<std::int32_t>(p_advice));
			return {};
		}

		auto memory_advise(memory_advice p_advice) -> expected<void, error_t>
		{
			if (!m_is_open)
			{
				return unexpected<error_t>(error_t::not_open);
			}

			if (!m_is_mmap || m_data == nullptr)
			{
				UTILS_DEBUG_LOG("file::memory_advise() - Not memory mapped\n");
				return unexpected<error_t>(error_t::not_open);
			}

			if (!apply_memory_advice(m_data, m_size, p_advice))
			{
				UTILS_DEBUG_LOG("file::memory_advise() - Failed to apply advice\n");
				return unexpected<error_t>(error_t::mmap_failed);
			}

			UTILS_DEBUG_LOG("file::memory_advise() - Applied memory advice: {}\n", static_cast<std::int32_t>(p_advice));
			return {};
		}

		auto read_all() -> expected<std::vector<byte_t>, error_t>
		{
			if (!m_is_open)
			{
				return unexpected<error_t>(error_t::not_open);
			}

			if (m_data != nullptr)
			{
				return std::vector<byte_t>(m_data, m_data + m_size);
			}

			std::vector<byte_t> buffer(m_size);
			auto saved_pos = m_position;
			m_position	   = 0;
			auto result	   = read(buffer.data(), m_size);
			m_position	   = saved_pos;

			if (!result)
			{
				return unexpected<error_t>(result.error());
			}

			return buffer;
		}

		auto read_text() -> expected<std::string, error_t>
		{
			if (!m_is_open)
			{
				return unexpected<error_t>(error_t::not_open);
			}

			if (m_data != nullptr)
			{
				return std::string(reinterpret_cast<const char*>(m_data), m_size);
			}

			std::string buffer(m_size, '\0');
			auto saved_pos = m_position;
			m_position	   = 0;
			auto result	   = read(&buffer[0], m_size);
			m_position	   = saved_pos;

			if (!result)
			{
				return unexpected<error_t>(result.error());
			}

			return buffer;
		}

		auto write_text(const std::string& p_text) -> expected<size_type, error_t> { return write(p_text.data(), p_text.size()); }

		auto eof() const -> bool { return m_position >= static_cast<pos_type>(m_size); }

		auto is_open() const -> bool { return m_is_open; }

		auto is_ram() const -> bool { return m_is_ram && m_data != nullptr; }

		auto is_mmap() const -> bool { return m_is_mmap; }

		auto get_path() const -> const fs::path& { return m_path; }

		auto get_mode() const -> mode_t { return m_mode; }
	};

	class file_exc : public std::runtime_error
	{
	public:
		explicit file_exc(const std::string& p_message) : std::runtime_error("Error: " + p_message) {}
	};

}	 // namespace utils
