#pragma once
#ifndef CMEMORY_HPP
	#define CMEMORY_HPP

	#include <algorithm>
	#include <cstddef>
	#include <cstdint>
	#include <cstring>
	#include <memory>
	#include <type_traits>
	#include <utility>

	#include "utils_macros.hpp"

	#if !MACRO_CXX14_ENABLED
namespace std
{
	template <typename in_t, typename... in_args_t>
	MACRO_NODISCARD MACRO_CONSTEXPR_FUNC auto make_unique(in_args_t&&... p_args) ->
		typename std::enable_if<!std::is_array<in_t>::value, std::unique_ptr<in_t> >::type
	{
		return std::unique_ptr<in_t>(new in_t(std::forward<in_args_t>(p_args)...));
	}

	template <typename in_t>
	MACRO_NODISCARD MACRO_CONSTEXPR_FUNC auto make_unique(std::size_t p_size) ->
		typename std::enable_if<std::is_array<in_t>::value && std::extent<in_t>::value == 0, std::unique_ptr<in_t> >::type
	{
		using element_t = typename std::remove_extent<in_t>::type;
		return std::unique_ptr<in_t>(new element_t[p_size]());
	}

	template <typename in_t, typename... in_args_t>
	MACRO_NODISCARD MACRO_CONSTEXPR_FUNC auto make_unique(in_args_t&&...) -> typename std::enable_if<std::extent<in_t>::value != 0>::type = delete;
}	 // namespace std
	#endif

namespace utils
{

	/**
	 * @brief Copy memory from p_src to p_dest
	 * @return pointer to destination
	 */
	inline auto mem_copy(void* p_dest, const void* p_src, std::size_t p_size) -> void*
	{
		return std::memcpy(p_dest, p_src, p_size);
	}

	/**
	 * @brief Move memory from p_src to p_dest (handles overlap)
	 * @return pointer to destination
	 */
	inline auto mem_move(void* p_dest, const void* p_src, std::size_t p_size) -> void*
	{
		return std::memmove(p_dest, p_src, p_size);
	}

	/**
	 * @brief Set memory region to given byte value
	 * @return pointer to destination
	 */
	inline auto mem_set(void* p_dest, std::int32_t p_value, std::size_t p_size) -> void*
	{
		return std::memset(p_dest, p_value, p_size);
	}

	/**
	 * @brief Zero memory region
	 * @return pointer to destination
	 */
	inline auto mem_zero(void* p_dest, std::size_t p_size) -> void*
	{
		return std::memset(p_dest, 0, p_size);
	}

	/**
	 * @brief Compare two memory regions
	 * @return negative, zero, or positive like std::memcmp
	 */
	inline auto mem_compare(const void* p_lhs, const void* p_rhs, std::size_t p_size) -> std::int32_t
	{
		return std::memcmp(p_lhs, p_rhs, p_size);
	}

	namespace cmemory
	{
		/**
		 * @brief Helper trait to obtain alignment of a type
		 */
		template <typename type_t> struct alignment_of
		{
			static constexpr std::size_t value = alignof(type_t);
		};

		/**
		 * @brief Metafunction to compute maximum alignment from a pack of values
		 */
		template <std::size_t... values> struct max_align;

		/**
		 * @brief Base case for max_align
		 */
		template <> struct max_align<>
		{
			static constexpr std::size_t value = 0U;
		};

		/**
		 * @brief Recursive case for max_align
		 */
		template <std::size_t first, std::size_t... rest> struct max_align<first, rest...>
		{
			static constexpr std::size_t value = (first > max_align<rest...>::value) ? first : max_align<rest...>::value;
		};

		/**
		 * @brief Align up a size to the given alignment (alignment must be power of two)
		 * @return aligned size
		 */
		inline auto align_up(std::size_t p_size, std::size_t p_alignment) -> std::size_t
		{
			return (p_size + (p_alignment - 1U)) & ~(p_alignment - 1U);
		}

		/**
		 * @brief Check if pointer is aligned to p_alignment (power of two)
		 * @return true if aligned
		 */
		inline auto is_aligned(const void* p_ptr, std::size_t p_alignment) -> bool
		{
			return (reinterpret_cast<std::uintptr_t>(p_ptr) & (p_alignment - 1U)) == 0U;
		}

		/**
		 * @brief Minimal aligned storage implementation for C++11
		 *
		 * Provides raw aligned storage and utilities to construct/destroy objects in-place.
		 */
		template <std::size_t len, std::size_t align = alignof(std::max_align_t)> class aligned_storage
		{
		public:
			/**
			 * @brief Type alias for this class
			 */
			using self_t = aligned_storage;

		private:
			static_assert((align & (align - 1U)) == 0U, "Alignment must be power of 2");

			static constexpr std::size_t storage_size = (len + (align - 1U)) & ~(align - 1U);

			alignas(align) unsigned char m_storage[storage_size];

		public:
			/**
			 * @brief Default destructor
			 */
			~aligned_storage() = default;

			/**
			 * @brief Default ctor clears storage to zero
			 */
			aligned_storage() { utils::mem_zero(m_storage, storage_size); }

			aligned_storage(const self_t&)			 = delete;
			auto operator=(const self_t&) -> self_t& = delete;
			aligned_storage(self_t&&)				 = delete;
			auto operator=(self_t&&) -> self_t&		 = delete;

		public:
			/**
			 * @brief Get pointer to storage
			 * @return void* pointer
			 */
			auto data() noexcept -> void* { return static_cast<void*>(m_storage); }

			/**
			 * @brief Get const pointer to storage
			 * @return const void* pointer
			 */
			auto data() const noexcept -> const void* { return static_cast<const void*>(m_storage); }

			/**
			 * @brief Get typed pointer to storage
			 * @tparam type_t target type
			 * @return pointer to type_t within storage
			 */
			template <typename type_t> auto as() noexcept -> type_t*
			{
				static_assert(sizeof(type_t) <= storage_size, "Type too large for storage");
				static_assert(alignof(type_t) <= align, "Type alignment exceeds storage alignment");
				return static_cast<type_t*>(data());
			}

			/**
			 * @brief Get typed const pointer to storage
			 * @tparam type_t target type
			 * @return const pointer to type_t within storage
			 */
			template <typename type_t> auto as() const noexcept -> const type_t*
			{
				static_assert(sizeof(type_t) <= storage_size, "Type too large for storage");
				static_assert(alignof(type_t) <= align, "Type alignment exceeds storage alignment");
				return static_cast<const type_t*>(data());
			}

			/**
			 * @brief Construct an object of type_t in storage with forwarded args
			 * @return pointer to constructed object
			 */
			template <typename type_t, typename... args_t> auto construct(args_t&&... p_args) -> type_t*
			{
				static_assert(sizeof(type_t) <= storage_size, "Type too large for storage");
				static_assert(alignof(type_t) <= align, "Type alignment exceeds storage alignment");
				return new (data()) type_t(std::forward<args_t>(p_args)...);
			}

			/**
			 * @brief Destroy object of type_t residing in storage
			 */
			template <typename type_t> auto destroy() noexcept -> void { as<type_t>()->~type_t(); }

			/**
			 * @brief Clear storage bytes to zero
			 */
			auto clear() noexcept -> void { utils::mem_zero(m_storage, storage_size); }

		public:
			/**
			 * @brief Get storage size in bytes
			 */
			static constexpr auto get_size() noexcept -> std::size_t { return storage_size; }

			/**
			 * @brief Get storage alignment
			 */
			static constexpr auto get_alignment() noexcept -> std::size_t { return align; }
		};

		/**
		 * @brief Type-based aligned storage with lifetime control
		 *
		 * Allows placement-construction and destruction of a single value_type.
		 */
		template <typename type_t> class typed_storage
		{
		public:
			using self_t	 = typed_storage;
			using value_type = type_t;

		private:
			aligned_storage<sizeof(type_t), alignof(type_t)> m_storage;
			bool m_initialized;

		public:
			/**
			 * @brief Destructor destroys stored value if present
			 */
			~typed_storage()
			{
				if (m_initialized)
				{
					destroy();
				}
			}

			/**
			 * @brief Default constructor initializes empty storage
			 */
			typed_storage() : m_initialized(false) {}

			typed_storage(const self_t&)			 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			/**
			 * @brief Move constructor moves contained value if present
			 */
			typed_storage(self_t&& p_other) noexcept : m_initialized(p_other.m_initialized)
			{
				if (m_initialized)
				{
					new (m_storage.data()) type_t(std::move(*p_other.m_storage.template as<type_t>()));
					p_other.destroy();
					p_other.m_initialized = false;
				}
			}

			/**
			 * @brief Move assignment operator moves contained value if present
			 */
			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					if (m_initialized)
					{
						destroy();
					}

					m_initialized = p_other.m_initialized;
					if (m_initialized)
					{
						new (m_storage.data()) type_t(std::move(*p_other.m_storage.template as<type_t>()));
						p_other.destroy();
						p_other.m_initialized = false;
					}
				}
				return *this;
			}

		public:
			/**
			 * @brief Construct value in place and return reference
			 * @note type_t constructor must not throw (no-exception policy)
			 */
			template <typename... args_t> auto emplace(args_t&&... p_args) -> type_t&
			{
				if (m_initialized)
				{
					destroy();
				}

				type_t* m_ptr = m_storage.template construct<type_t>(std::forward<args_t>(p_args)...);
				m_initialized = true;
				return *m_ptr;
			}

			/**
			 * @brief Destroy stored value if present
			 */
			auto destroy() noexcept -> void
			{
				if (m_initialized)
				{
					m_storage.template destroy<type_t>();
					m_initialized = false;
				}
			}

			/**
			 * @brief Check if value is present
			 * @return true if initialized
			 */
			auto has_value() const noexcept -> bool { return m_initialized; }

			/**
			 * @brief Get stored value (non-const)
			 */
			auto get() -> type_t& { return *m_storage.template as<type_t>(); }

			/**
			 * @brief Get stored value (const)
			 */
			auto get() const -> const type_t& { return *m_storage.template as<type_t>(); }

			/**
			 * @brief Dereference operator
			 */
			auto operator*() -> type_t& { return get(); }

			/**
			 * @brief Dereference operator (const)
			 */
			auto operator*() const -> const type_t& { return get(); }

			/**
			 * @brief Arrow operator
			 */
			auto operator->() -> type_t* { return m_storage.template as<type_t>(); }

			/**
			 * @brief Arrow operator (const)
			 */
			auto operator->() const -> const type_t* { return m_storage.template as<type_t>(); }
		};

		/**
		 * @brief Fixed-size memory pool for type_t (not thread-safe)
		 *
		 * Provides allocate/construct/deallocate/destroy operations for objects of type_t.
		 */
		template <typename type_t, std::size_t pool_size = 32U> class memory_pool
		{
		public:
			using self_t	 = memory_pool;
			using value_type = type_t;

		private:
			struct block
			{
				aligned_storage<sizeof(type_t), alignof(type_t)> storage;
				block* next;

				block() : next(nullptr) {}
			};

			static_assert(pool_size > 0U, "pool_size must be > 0");

			block m_blocks[pool_size];
			block* m_free_list;
			std::size_t m_allocated;

		public:
			/**
			 * @brief Default destructor
			 */
			~memory_pool() = default;

			/**
			 * @brief Construct and initialize pool
			 */
			memory_pool() : m_free_list(nullptr), m_allocated(0U) { reset(); }

			memory_pool(const self_t&)				 = delete;
			auto operator=(const self_t&) -> self_t& = delete;
			memory_pool(self_t&&)					 = delete;
			auto operator=(self_t&&) -> self_t&		 = delete;

		private:
			/**
			 * @brief Reset pool free-list to initial state
			 */
			auto reset() -> void
			{
				m_free_list = &m_blocks[0];
				for (std::size_t idx_for = 0U; idx_for + 1U < pool_size; ++idx_for)
				{
					m_blocks[idx_for].next = &m_blocks[idx_for + 1U];
				}
				m_blocks[pool_size - 1U].next = nullptr;
				m_allocated					  = 0U;
			}

		public:
			/**
			 * @brief Allocate raw storage for one object (unconstructed)
			 * @return pointer to raw storage or nullptr if exhausted
			 */
			auto allocate() -> type_t*
			{
				if (m_free_list == nullptr)
				{
					return nullptr;
				}

				block* m_blk = m_free_list;
				m_free_list	 = m_blk->next;
				++m_allocated;

				return m_blk->storage.template as<type_t>();
			}

			/**
			 * @brief Construct object in a pool slot with forwarded args
			 * @return pointer to constructed object or nullptr
			 */
			template <typename... args_t> auto construct(args_t&&... p_args) -> type_t*
			{
				type_t* m_ptr = allocate();
				if (m_ptr != nullptr)
				{
					new (m_ptr) type_t(std::forward<args_t>(p_args)...);
				}
				return m_ptr;
			}

			/**
			 * @brief Return raw slot to pool (does not call destructor)
			 */
			auto deallocate(type_t* p_ptr) noexcept -> void
			{
				if (p_ptr == nullptr)
				{
					return;
				}

				for (std::size_t idx_for = 0U; idx_for < pool_size; ++idx_for)
				{
					if (m_blocks[idx_for].storage.template as<type_t>() == p_ptr)
					{
						m_blocks[idx_for].next = m_free_list;
						m_free_list			   = &m_blocks[idx_for];
						if (m_allocated > 0U)
						{
							--m_allocated;
						}
						return;
					}
				}
			}

			/**
			 * @brief Destroy object and return slot to pool
			 */
			auto destroy(type_t* p_ptr) noexcept -> void
			{
				if (p_ptr != nullptr)
				{
					p_ptr->~type_t();
					deallocate(p_ptr);
				}
			}

			/**
			 * @brief Check if pool has any free slot
			 */
			auto has_space() const noexcept -> bool { return m_free_list != nullptr; }

			/**
			 * @brief Get number of allocated (checked-out) slots
			 */
			auto get_allocated() const noexcept -> std::size_t { return m_allocated; }

			/**
			 * @brief Get number of available slots
			 */
			auto get_available() const noexcept -> std::size_t { return pool_size - m_allocated; }
		};

		/**
		 * @brief Minimal scoped pointer providing unique ownership
		 *
		 * Prefer std::unique_ptr in new code. Provided for API compatibility.
		 */
		template <typename type_t> class scoped_ptr
		{
		public:
			using self_t	 = scoped_ptr;
			using value_type = type_t;
			using pointer	 = type_t*;

		private:
			pointer m_ptr;

		public:
			/**
			 * @brief Destructor deletes owned pointer
			 */
			~scoped_ptr() { delete m_ptr; }

			/**
			 * @brief Default ctor initializes to nullptr
			 */
			scoped_ptr() : m_ptr(nullptr) {}

			/**
			 * @brief Construct from raw pointer
			 */
			explicit scoped_ptr(pointer p_ptr) : m_ptr(p_ptr) {}

			scoped_ptr(const self_t&)				 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			/**
			 * @brief Move constructor transfers ownership
			 */
			scoped_ptr(self_t&& p_other) noexcept : m_ptr(p_other.m_ptr) { p_other.m_ptr = nullptr; }

			/**
			 * @brief Move assignment transfers ownership
			 */
			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					delete m_ptr;
					m_ptr		  = p_other.m_ptr;
					p_other.m_ptr = nullptr;
				}
				return *this;
			}

		public:
			/**
			 * @brief Reset owned pointer (deletes previous)
			 */
			auto reset(pointer p_ptr = nullptr) -> void
			{
				delete m_ptr;
				m_ptr = p_ptr;
			}

			/**
			 * @brief Release ownership and return raw pointer
			 */
			auto release() noexcept -> pointer
			{
				pointer m_tmp = m_ptr;
				m_ptr		  = nullptr;
				return m_tmp;
			}

			/**
			 * @brief Get raw pointer
			 */
			auto get() const noexcept -> pointer { return m_ptr; }

			/**
			 * @brief Dereference operator
			 */
			auto operator*() const -> type_t& { return *m_ptr; }

			/**
			 * @brief Arrow operator
			 */
			auto operator->() const noexcept -> pointer { return m_ptr; }

			/**
			 * @brief Bool conversion checks non-null
			 */
			explicit operator bool() const noexcept { return m_ptr != nullptr; }
		};

		/**
		 * @brief Fixed-size, aligned byte buffer
		 *
		 * Provides write/read/resize semantics into internal aligned storage.
		 */
		template <std::size_t size, std::size_t align = alignof(std::max_align_t)> class aligned_buffer
		{
		public:
			using self_t = aligned_buffer;

		private:
			aligned_storage<size, align> m_storage;
			std::size_t m_used;

		public:
			/**
			 * @brief Default destructor
			 */
			~aligned_buffer() = default;

			/**
			 * @brief Default constructor initializes empty buffer
			 */
			aligned_buffer() : m_used(0U) {}

			aligned_buffer(const self_t&)			 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			/**
			 * @brief Move constructor copies used data and clears source
			 */
			aligned_buffer(self_t&& p_other) noexcept : m_used(p_other.m_used)
			{
				if (m_used > 0U)
				{
					utils::mem_copy(m_storage.data(), p_other.m_storage.data(), m_used);
				}
				p_other.m_used = 0U;
			}

			/**
			 * @brief Move assignment copies used data and clears source
			 */
			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					if (p_other.m_used > 0U)
					{
						utils::mem_copy(m_storage.data(), p_other.m_storage.data(), p_other.m_used);
					}
					m_used		   = p_other.m_used;
					p_other.m_used = 0U;
				}
				return *this;
			}

		public:
			/**
			 * @brief Write p_size bytes from p_data to buffer (append)
			 * @return true on success, false if not enough space
			 */
			auto write(const void* p_data, std::size_t p_size) -> bool
			{
				if (m_used + p_size > size)
				{
					return false;
				}

				utils::mem_copy(static_cast<char*>(m_storage.data()) + m_used, p_data, p_size);
				m_used += p_size;
				return true;
			}

			/**
			 * @brief Read p_size bytes into p_dest from offset p_offset
			 * @return true on success, false on out-of-range
			 */
			auto read(void* p_dest, std::size_t p_size, std::size_t p_offset = 0U) const -> bool
			{
				if (p_offset + p_size > m_used)
				{
					return false;
				}

				utils::mem_copy(p_dest, static_cast<const char*>(m_storage.data()) + p_offset, p_size);
				return true;
			}

			/**
			 * @brief Clear buffer to zero and reset used size
			 */
			auto clear() noexcept -> void
			{
				m_storage.clear();
				m_used = 0U;
			}

			/**
			 * @brief Get pointer to buffer data
			 */
			auto data() noexcept -> void* { return m_storage.data(); }

			/**
			 * @brief Get const pointer to buffer data
			 */
			auto data() const noexcept -> const void* { return m_storage.data(); }

			/**
			 * @brief Resize used size (must be <= capacity)
			 * @return true on success
			 */
			auto resize(std::size_t p_new_size) noexcept -> bool
			{
				if (p_new_size > size)
				{
					return false;
				}
				m_used = p_new_size;
				return true;
			}

		public:
			/**
			 * @brief Get capacity in bytes
			 */
			auto get_size() const noexcept -> std::size_t { return size; }

			/**
			 * @brief Get currently used bytes
			 */
			auto get_used() const noexcept -> std::size_t { return m_used; }

			/**
			 * @brief Get remaining free bytes
			 */
			auto get_available() const noexcept -> std::size_t { return size - m_used; }

			/**
			 * @brief Get alignment of buffer
			 */
			auto get_alignment() const noexcept -> std::size_t { return align; }
		};

	}	 // namespace cmemory
}	 // namespace utils

#endif	  // CMEMORY_HPP
