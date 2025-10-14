#ifndef OPTIONAL_HPP
#define OPTIONAL_HPP

#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <utility>

#include "concepts.hpp"
#include "utility_types.hpp"

namespace utils
{

	// Forward declarations
	template <typename val_t> class optional;

	/**
	 * @brief Tag type for nullopt
	 */
	struct nullopt_t
	{
		explicit nullopt_t() = default;
	};

	constexpr nullopt_t nullopt{};

	/**
	 * @brief Main optional template for nullable value semantics
	 * @tparam val_t Type of the optional value
	 */
	template <typename val_t> class optional
	{
		static_assert(!std::is_reference<val_t>::value, "Optional cannot hold references");
		static_assert(!std::is_void<val_t>::value, "Optional cannot hold void");
		static_assert(is_destructible<val_t>::value, "Value type must be destructible");

	public:
		using self_t	 = optional;
		using value_type = val_t;

	private:
		union storage_t
		{
			std::uint8_t m_dummy;
			val_t m_value;

			storage_t() : m_dummy(0) {}

			~storage_t() {}
		} m_storage;

		bool m_has_value;

	public:
		~optional()
		{
			if (m_has_value)
			{
				m_storage.m_value.~val_t();
			}
		}

		optional() noexcept : m_has_value(false) { m_storage.m_dummy = 0; }

		optional(nullopt_t) noexcept : m_has_value(false) { m_storage.m_dummy = 0; }

		optional(const val_t& p_val) : m_has_value(true) { new (&m_storage.m_value) val_t(p_val); }

		optional(val_t&& p_val) : m_has_value(true) { new (&m_storage.m_value) val_t(std::move(p_val)); }

		template <typename... args_t> explicit optional(in_place_t, args_t&&... p_args) : m_has_value(true)
		{
			new (&m_storage.m_value) val_t(std::forward<args_t>(p_args)...);
		}

		template <typename init_t, typename... args_t>
		explicit optional(in_place_t, std::initializer_list<init_t> p_il, args_t&&... p_args) : m_has_value(true)
		{
			new (&m_storage.m_value) val_t(p_il, std::forward<args_t>(p_args)...);
		}

		optional(const self_t& p_other) : m_has_value(p_other.m_has_value)
		{
			if (m_has_value)
			{
				new (&m_storage.m_value) val_t(p_other.m_storage.m_value);
			}
			else
			{
				m_storage.m_dummy = 0;
			}
		}

		auto operator=(const self_t& p_other) -> self_t&
		{
			if (this != &p_other)
			{
				if (m_has_value && p_other.m_has_value)
				{
					m_storage.m_value = p_other.m_storage.m_value;
				}
				else if (!m_has_value && p_other.m_has_value)
				{
					new (&m_storage.m_value) val_t(p_other.m_storage.m_value);
					m_has_value = true;
				}
				else if (m_has_value && !p_other.m_has_value)
				{
					m_storage.m_value.~val_t();
					m_storage.m_dummy = 0;
					m_has_value		  = false;
				}
			}
			return *this;
		}

		optional(self_t&& p_other) noexcept(is_nothrow_move_constructible<val_t>::value) : m_has_value(p_other.m_has_value)
		{
			if (m_has_value)
			{
				new (&m_storage.m_value) val_t(std::move(p_other.m_storage.m_value));
			}
			else
			{
				m_storage.m_dummy = 0;
			}
		}

		auto operator=(self_t&& p_other) noexcept(is_nothrow_move_constructible<val_t>::value) -> self_t&
		{
			if (this != &p_other)
			{
				if (m_has_value && p_other.m_has_value)
				{
					m_storage.m_value = std::move(p_other.m_storage.m_value);
				}
				else if (!m_has_value && p_other.m_has_value)
				{
					new (&m_storage.m_value) val_t(std::move(p_other.m_storage.m_value));
					m_has_value = true;
				}
				else if (m_has_value && !p_other.m_has_value)
				{
					m_storage.m_value.~val_t();
					m_storage.m_dummy = 0;
					m_has_value		  = false;
				}
			}
			return *this;
		}

		auto operator=(nullopt_t) noexcept -> self_t&
		{
			reset();
			return *this;
		}

		template <typename val2_t>
		auto operator=(val2_t&& p_val) -> typename enable_if<conjunction<negation<is_same<remove_cvref_t<val2_t>, self_t> >,
																		 std::is_constructible<val_t, val2_t>,
																		 std::is_assignable<val_t&, val2_t> >::value,
															 self_t&>::type
		{
			if (m_has_value)
			{
				m_storage.m_value = std::forward<val2_t>(p_val);
			}
			else
			{
				new (&m_storage.m_value) val_t(std::forward<val2_t>(p_val));
				m_has_value = true;
			}
			return *this;
		}

	public:
		auto has_value() const noexcept -> bool { return m_has_value; }

		explicit operator bool() const noexcept { return m_has_value; }

		auto value() & -> val_t& { return m_storage.m_value; }

		auto value() const& -> const val_t& { return m_storage.m_value; }

		auto value() && -> val_t&& { return std::move(m_storage.m_value); }

		auto value() const&& -> const val_t&& { return std::move(m_storage.m_value); }

		auto operator*() & noexcept -> val_t& { return m_storage.m_value; }

		auto operator*() const& noexcept -> const val_t& { return m_storage.m_value; }

		auto operator*() && noexcept -> val_t&& { return std::move(m_storage.m_value); }

		auto operator*() const&& noexcept -> const val_t&& { return std::move(m_storage.m_value); }

		auto operator->() noexcept -> val_t* { return &m_storage.m_value; }

		auto operator->() const noexcept -> const val_t* { return &m_storage.m_value; }

		template <typename def_t> auto value_or(def_t&& p_default) const& -> val_t
		{
			if (m_has_value)
			{
				return m_storage.m_value;
			}
			return static_cast<val_t>(std::forward<def_t>(p_default));
		}

		template <typename def_t> auto value_or(def_t&& p_default) && -> val_t
		{
			if (m_has_value)
			{
				return std::move(m_storage.m_value);
			}
			return static_cast<val_t>(std::forward<def_t>(p_default));
		}

		template <typename... args_t> auto emplace(args_t&&... p_args) -> val_t&
		{
			reset();
			new (&m_storage.m_value) val_t(std::forward<args_t>(p_args)...);
			m_has_value = true;
			return m_storage.m_value;
		}

		template <typename init_t, typename... args_t> auto emplace(std::initializer_list<init_t> p_il, args_t&&... p_args) -> val_t&
		{
			reset();
			new (&m_storage.m_value) val_t(p_il, std::forward<args_t>(p_args)...);
			m_has_value = true;
			return m_storage.m_value;
		}

		auto reset() noexcept -> void
		{
			if (m_has_value)
			{
				m_storage.m_value.~val_t();
				m_storage.m_dummy = 0;
				m_has_value		  = false;
			}
		}

		auto swap(self_t& p_other) noexcept(is_nothrow_move_constructible<val_t>::value
											&& noexcept(std::swap(std::declval<val_t&>(), std::declval<val_t&>()))) -> void
		{
			if (m_has_value && p_other.m_has_value)
			{
				using std::swap;
				swap(m_storage.m_value, p_other.m_storage.m_value);
			}
			else if (m_has_value && !p_other.m_has_value)
			{
				new (&p_other.m_storage.m_value) val_t(std::move(m_storage.m_value));
				m_storage.m_value.~val_t();
				m_storage.m_dummy	= 0;
				p_other.m_has_value = true;
				m_has_value			= false;
			}
			else if (!m_has_value && p_other.m_has_value)
			{
				new (&m_storage.m_value) val_t(std::move(p_other.m_storage.m_value));
				p_other.m_storage.m_value.~val_t();
				p_other.m_storage.m_dummy = 0;
				m_has_value				  = true;
				p_other.m_has_value		  = false;
			}
		}
	};

	/**
	 * @brief Comparison operators
	 */
	template <typename val_t> auto operator==(const optional<val_t>& p_lhs, const optional<val_t>& p_rhs) -> bool
	{
		if (p_lhs.has_value() != p_rhs.has_value())
		{
			return false;
		}
		if (!p_lhs.has_value())
		{
			return true;
		}
		return *p_lhs == *p_rhs;
	}

	template <typename val_t> auto operator!=(const optional<val_t>& p_lhs, const optional<val_t>& p_rhs) -> bool
	{
		return !(p_lhs == p_rhs);
	}

	template <typename val_t> auto operator<(const optional<val_t>& p_lhs, const optional<val_t>& p_rhs) -> bool
	{
		if (!p_rhs.has_value())
		{
			return false;
		}
		if (!p_lhs.has_value())
		{
			return true;
		}
		return *p_lhs < *p_rhs;
	}

	template <typename val_t> auto operator<=(const optional<val_t>& p_lhs, const optional<val_t>& p_rhs) -> bool
	{
		return !(p_rhs < p_lhs);
	}

	template <typename val_t> auto operator>(const optional<val_t>& p_lhs, const optional<val_t>& p_rhs) -> bool
	{
		return p_rhs < p_lhs;
	}

	template <typename val_t> auto operator>=(const optional<val_t>& p_lhs, const optional<val_t>& p_rhs) -> bool
	{
		return !(p_lhs < p_rhs);
	}

	/**
	 * @brief Comparison with nullopt
	 */
	template <typename val_t> auto operator==(const optional<val_t>& p_opt, nullopt_t) noexcept -> bool
	{
		return !p_opt.has_value();
	}

	template <typename val_t> auto operator==(nullopt_t, const optional<val_t>& p_opt) noexcept -> bool
	{
		return !p_opt.has_value();
	}

	template <typename val_t> auto operator!=(const optional<val_t>& p_opt, nullopt_t) noexcept -> bool
	{
		return p_opt.has_value();
	}

	template <typename val_t> auto operator!=(nullopt_t, const optional<val_t>& p_opt) noexcept -> bool
	{
		return p_opt.has_value();
	}

	template <typename val_t> auto operator<(const optional<val_t>&, nullopt_t) noexcept -> bool
	{
		return false;
	}

	template <typename val_t> auto operator<(nullopt_t, const optional<val_t>& p_opt) noexcept -> bool
	{
		return p_opt.has_value();
	}

	template <typename val_t> auto operator<=(const optional<val_t>& p_opt, nullopt_t) noexcept -> bool
	{
		return !p_opt.has_value();
	}

	template <typename val_t> auto operator<=(nullopt_t, const optional<val_t>&) noexcept -> bool
	{
		return true;
	}

	template <typename val_t> auto operator>(const optional<val_t>& p_opt, nullopt_t) noexcept -> bool
	{
		return p_opt.has_value();
	}

	template <typename val_t> auto operator>(nullopt_t, const optional<val_t>&) noexcept -> bool
	{
		return false;
	}

	template <typename val_t> auto operator>=(const optional<val_t>&, nullopt_t) noexcept -> bool
	{
		return true;
	}

	template <typename val_t> auto operator>=(nullopt_t, const optional<val_t>& p_opt) noexcept -> bool
	{
		return !p_opt.has_value();
	}

	/**
	 * @brief Comparison with value
	 */
	template <typename val_t, typename val2_t> auto operator==(const optional<val_t>& p_opt, const val2_t& p_val) -> bool
	{
		return p_opt.has_value() && (*p_opt == p_val);
	}

	template <typename val_t, typename val2_t> auto operator==(const val2_t& p_val, const optional<val_t>& p_opt) -> bool
	{
		return p_opt.has_value() && (p_val == *p_opt);
	}

	template <typename val_t, typename val2_t> auto operator!=(const optional<val_t>& p_opt, const val2_t& p_val) -> bool
	{
		return !p_opt.has_value() || !(*p_opt == p_val);
	}

	template <typename val_t, typename val2_t> auto operator!=(const val2_t& p_val, const optional<val_t>& p_opt) -> bool
	{
		return !p_opt.has_value() || !(p_val == *p_opt);
	}

	template <typename val_t, typename val2_t> auto operator<(const optional<val_t>& p_opt, const val2_t& p_val) -> bool
	{
		return !p_opt.has_value() || (*p_opt < p_val);
	}

	template <typename val_t, typename val2_t> auto operator<(const val2_t& p_val, const optional<val_t>& p_opt) -> bool
	{
		return p_opt.has_value() && (p_val < *p_opt);
	}

	template <typename val_t, typename val2_t> auto operator<=(const optional<val_t>& p_opt, const val2_t& p_val) -> bool
	{
		return !p_opt.has_value() || !(*p_opt > p_val);
	}

	template <typename val_t, typename val2_t> auto operator<=(const val2_t& p_val, const optional<val_t>& p_opt) -> bool
	{
		return p_opt.has_value() && !(p_val > *p_opt);
	}

	template <typename val_t, typename val2_t> auto operator>(const optional<val_t>& p_opt, const val2_t& p_val) -> bool
	{
		return p_opt.has_value() && (*p_opt > p_val);
	}

	template <typename val_t, typename val2_t> auto operator>(const val2_t& p_val, const optional<val_t>& p_opt) -> bool
	{
		return !p_opt.has_value() || (p_val > *p_opt);
	}

	template <typename val_t, typename val2_t> auto operator>=(const optional<val_t>& p_opt, const val2_t& p_val) -> bool
	{
		return p_opt.has_value() && !(*p_opt < p_val);
	}

	template <typename val_t, typename val2_t> auto operator>=(const val2_t& p_val, const optional<val_t>& p_opt) -> bool
	{
		return !p_opt.has_value() || !(p_val < *p_opt);
	}

	/**
	 * @brief Swap function
	 */
	template <typename val_t> auto swap(optional<val_t>& p_lhs, optional<val_t>& p_rhs) noexcept(noexcept(p_lhs.swap(p_rhs))) -> void
	{
		p_lhs.swap(p_rhs);
	}

	/**
	 * @brief Helper function to create optional value
	 */
	template <typename val_t> auto make_optional(val_t&& p_val) -> optional<decay_t<val_t> >
	{
		return optional<decay_t<val_t> >(std::forward<val_t>(p_val));
	}

	template <typename val_t, typename... args_t> auto make_optional(args_t&&... p_args) -> optional<val_t>
	{
		return optional<val_t>(in_place, std::forward<args_t>(p_args)...);
	}

	template <typename val_t, typename init_t, typename... args_t>
	auto make_optional(std::initializer_list<init_t> p_il, args_t&&... p_args) -> optional<val_t>
	{
		return optional<val_t>(in_place, p_il, std::forward<args_t>(p_args)...);
	}

}	 // namespace utils

#endif	  // OPTIONAL_HPP
