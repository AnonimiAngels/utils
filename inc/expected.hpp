#ifndef EXPECTED_HPP
#define EXPECTED_HPP

#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <utility>

#include "concepts.hpp"
#include "utility_types.hpp"

namespace utils
{

	// Forward declarations
	template <typename val_t, typename err_t> class expected;

	/**
	 * @brief Tag type for unexpected value construction
	 */
	struct unexpect_t
	{
		explicit unexpect_t() = default;
	};

	constexpr unexpect_t unexpect{};

	/**
	 * @brief Wrapper for unexpected error values
	 * @tparam err_t Type of the error value
	 */
	template <typename err_t> class unexpected
	{
		static_assert(is_valid_expected_error<err_t>::value, "Error type must be destructible object type, not array, const or volatile");

	public:
		using self_t	 = unexpected;
		using error_type = err_t;

	private:
		err_t m_error;

	public:
		~unexpected() = default;

		unexpected() = delete;

		explicit unexpected(const err_t& p_err) : m_error(p_err) {}

		explicit unexpected(err_t&& p_err) : m_error(std::move(p_err)) {}

		template <typename... args_t> explicit unexpected(in_place_t, args_t&&... p_args) : m_error(std::forward<args_t>(p_args)...) {}

		template <typename init_t, typename... args_t>
		explicit unexpected(in_place_t, std::initializer_list<init_t> p_il, args_t&&... p_args) : m_error(p_il, std::forward<args_t>(p_args)...)
		{
		}

		unexpected(const self_t&)				 = default;
		auto operator=(const self_t&) -> self_t& = default;

		unexpected(self_t&&)				= default;
		auto operator=(self_t&&) -> self_t& = default;

	public:
		auto error() const& -> const err_t& { return m_error; }

		auto error() & -> err_t& { return m_error; }

		auto error() const&& -> const err_t&& { return std::move(m_error); }

		auto error() && -> err_t&& { return std::move(m_error); }

	public:
		auto swap(self_t& p_other) -> void
		{
			using std::swap;
			swap(m_error, p_other.m_error);
		}
	};

	/**
	 * @brief Main expected template for error handling without exceptions
	 * @tparam val_t Type of the expected value
	 * @tparam err_t Type of the error value
	 */
	template <typename val_t, typename err_t> class expected
	{
		static_assert(is_valid_expected_error<err_t>::value, "Error type must be destructible object type, not array, const or volatile");
		static_assert(!std::is_void<val_t>::value, "Use expected<void, E> specialization for void value type");

	public:
		using self_t		  = expected;
		using value_type	  = val_t;
		using error_type	  = err_t;
		using unexpected_type = unexpected<err_t>;

	private:
		union storage_t
		{
			val_t m_value;
			err_t m_error;

			storage_t() {}

			~storage_t() {}
		} m_storage;

		bool m_has_value;

	public:
		~expected()
		{
			if (m_has_value)
			{
				m_storage.m_value.~val_t();
			}
			else
			{
				m_storage.m_error.~err_t();
			}
		}

		expected() : m_has_value(true) { new (&m_storage.m_value) val_t(); }

		expected(const val_t& p_val) : m_has_value(true) { new (&m_storage.m_value) val_t(p_val); }

		expected(val_t&& p_val) : m_has_value(true) { new (&m_storage.m_value) val_t(std::move(p_val)); }

		template <typename... args_t> explicit expected(in_place_t, args_t&&... p_args) : m_has_value(true)
		{
			new (&m_storage.m_value) val_t(std::forward<args_t>(p_args)...);
		}

		template <typename init_t, typename... args_t>
		explicit expected(in_place_t, std::initializer_list<init_t> p_il, args_t&&... p_args) : m_has_value(true)
		{
			new (&m_storage.m_value) val_t(p_il, std::forward<args_t>(p_args)...);
		}

		expected(const unexpected_type& p_unex) : m_has_value(false) { new (&m_storage.m_error) err_t(p_unex.error()); }

		expected(unexpected_type&& p_unex) : m_has_value(false) { new (&m_storage.m_error) err_t(std::move(p_unex.error())); }

		template <typename... args_t> explicit expected(unexpect_t, args_t&&... p_args) : m_has_value(false)
		{
			new (&m_storage.m_error) err_t(std::forward<args_t>(p_args)...);
		}

		template <typename init_t, typename... args_t>
		explicit expected(unexpect_t, std::initializer_list<init_t> p_il, args_t&&... p_args) : m_has_value(false)
		{
			new (&m_storage.m_error) err_t(p_il, std::forward<args_t>(p_args)...);
		}

		expected(const self_t& p_other) : m_has_value(p_other.m_has_value)
		{
			if (m_has_value)
			{
				new (&m_storage.m_value) val_t(p_other.m_storage.m_value);
			}
			else
			{
				new (&m_storage.m_error) err_t(p_other.m_storage.m_error);
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
				else if (!m_has_value && !p_other.m_has_value)
				{
					m_storage.m_error = p_other.m_storage.m_error;
				}
				else
				{
					this->~expected();
					new (this) self_t(p_other);
				}
			}
			return *this;
		}

		expected(self_t&& p_other) noexcept(is_nothrow_move_constructible<val_t>::value && is_nothrow_move_constructible<err_t>::value)
			: m_has_value(p_other.m_has_value)
		{
			if (m_has_value)
			{
				new (&m_storage.m_value) val_t(std::move(p_other.m_storage.m_value));
			}
			else
			{
				new (&m_storage.m_error) err_t(std::move(p_other.m_storage.m_error));
			}
		}

		auto operator=(self_t&& p_other) noexcept(is_nothrow_move_constructible<val_t>::value && is_nothrow_move_constructible<err_t>::value)
			-> self_t&
		{
			if (this != &p_other)
			{
				if (m_has_value && p_other.m_has_value)
				{
					m_storage.m_value = std::move(p_other.m_storage.m_value);
				}
				else if (!m_has_value && !p_other.m_has_value)
				{
					m_storage.m_error = std::move(p_other.m_storage.m_error);
				}
				else
				{
					this->~expected();
					new (this) self_t(std::move(p_other));
				}
			}
			return *this;
		}

		template <typename val2_t>
		auto operator=(val2_t&& p_val) -> typename enable_if<conjunction<negation<is_same<remove_cvref_t<val2_t>, self_t> >,
																		 negation<is_same<remove_cvref_t<val2_t>, unexpected<err_t> > >,
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
				m_storage.m_error.~err_t();
				new (&m_storage.m_value) val_t(std::forward<val2_t>(p_val));
				m_has_value = true;
			}
			return *this;
		}

		template <typename err2_t>
		auto operator=(const unexpected<err2_t>& p_unex) -> typename enable_if<std::is_assignable<err_t&, const err2_t&>::value, self_t&>::type
		{
			if (m_has_value)
			{
				m_storage.m_value.~val_t();
				new (&m_storage.m_error) err_t(p_unex.error());
				m_has_value = false;
			}
			else
			{
				m_storage.m_error = p_unex.error();
			}
			return *this;
		}

		template <typename err2_t> auto operator=(unexpected<err2_t>&& p_unex) -> self_t&
		{
			static_assert(std::is_assignable<err_t&, err2_t>::value, "Error type must be assignable");
			if (m_has_value)
			{
				m_storage.m_value.~val_t();
				new (&m_storage.m_error) err_t(std::move(p_unex.error()));
				m_has_value = false;
			}
			else
			{
				m_storage.m_error = std::move(p_unex.error());
			}
			return *this;
		}

	public:
		template <typename... args_t> auto emplace(args_t&&... p_args) -> val_t&
		{
			if (m_has_value)
			{
				m_storage.m_value.~val_t();
			}
			else
			{
				m_storage.m_error.~err_t();
				m_has_value = true;
			}
			new (&m_storage.m_value) val_t(std::forward<args_t>(p_args)...);
			return m_storage.m_value;
		}

		template <typename init_t, typename... args_t> auto emplace(std::initializer_list<init_t> p_il, args_t&&... p_args) -> val_t&
		{
			if (m_has_value)
			{
				m_storage.m_value.~val_t();
			}
			else
			{
				m_storage.m_error.~err_t();
				m_has_value = true;
			}
			new (&m_storage.m_value) val_t(p_il, std::forward<args_t>(p_args)...);
			return m_storage.m_value;
		}

	public:
		auto has_value() const noexcept -> bool { return m_has_value; }

		explicit operator bool() const noexcept { return m_has_value; }

		auto value() & noexcept -> val_t& { return m_storage.m_value; }

		auto value() const& -> const val_t& { return m_storage.m_value; }

		auto value() && -> val_t&& { return std::move(m_storage.m_value); }

		auto value() const&& -> const val_t&& { return std::move(m_storage.m_value); }

		auto operator*() & noexcept -> val_t& { return m_storage.m_value; }

		auto operator*() const& noexcept -> const val_t& { return m_storage.m_value; }

		auto operator*() && noexcept -> val_t&& { return std::move(m_storage.m_value); }

		auto operator*() const&& noexcept -> const val_t&& { return std::move(m_storage.m_value); }

		auto operator->() noexcept -> val_t* { return &m_storage.m_value; }

		auto operator->() const noexcept -> const val_t* { return &m_storage.m_value; }

		auto error() & noexcept -> err_t& { return m_storage.m_error; }

		auto error() const& noexcept -> const err_t& { return m_storage.m_error; }

		auto error() && noexcept -> err_t&& { return std::move(m_storage.m_error); }

		auto error() const&& noexcept -> const err_t&& { return std::move(m_storage.m_error); }

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

	public:
		auto swap(self_t& p_other) noexcept -> void
		{
			if (m_has_value && p_other.m_has_value)
			{
				using std::swap;
				swap(m_storage.m_value, p_other.m_storage.m_value);
			}
			else if (!m_has_value && !p_other.m_has_value)
			{
				using std::swap;
				swap(m_storage.m_error, p_other.m_storage.m_error);
			}
			else
			{
				self_t tmp(std::move(*this));
				*this	= std::move(p_other);
				p_other = std::move(tmp);
			}
		}
	};

	/**
	 * @brief Specialization for void value type
	 * @tparam err_t Type of the error value
	 */
	template <typename err_t> class expected<void, err_t>
	{
		static_assert(is_valid_expected_error<err_t>::value, "Error type must be destructible object type, not array, const or volatile");

	public:
		using self_t		  = expected;
		using value_type	  = void;
		using error_type	  = err_t;
		using unexpected_type = unexpected<err_t>;

	private:
		union storage_t
		{
			std::uint8_t m_dummy;
			err_t m_error;

			storage_t() : m_dummy(0) {}

			~storage_t() {}
		} m_storage;

		bool m_has_value;

	public:
		~expected()
		{
			if (!m_has_value)
			{
				m_storage.m_error.~err_t();
			}
		}

		expected() noexcept : m_has_value(true) { m_storage.m_dummy = 0; }

		explicit expected(in_place_t) noexcept : m_has_value(true) { m_storage.m_dummy = 0; }

		expected(const unexpected_type& p_unex) : m_has_value(false) { new (&m_storage.m_error) err_t(p_unex.error()); }

		expected(unexpected_type&& p_unex) : m_has_value(false) { new (&m_storage.m_error) err_t(std::move(p_unex.error())); }

		template <typename... args_t> explicit expected(unexpect_t, args_t&&... p_args) : m_has_value(false)
		{
			new (&m_storage.m_error) err_t(std::forward<args_t>(p_args)...);
		}

		template <typename init_t, typename... args_t>
		explicit expected(unexpect_t, std::initializer_list<init_t> p_il, args_t&&... p_args) : m_has_value(false)
		{
			new (&m_storage.m_error) err_t(p_il, std::forward<args_t>(p_args)...);
		}

		expected(const self_t& p_other) : m_has_value(p_other.m_has_value)
		{
			if (!m_has_value)
			{
				new (&m_storage.m_error) err_t(p_other.m_storage.m_error);
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
				if (!m_has_value && !p_other.m_has_value)
				{
					m_storage.m_error = p_other.m_storage.m_error;
				}
				else if (m_has_value != p_other.m_has_value)
				{
					this->~expected();
					new (this) self_t(p_other);
				}
			}
			return *this;
		}

		expected(self_t&& p_other) noexcept(is_nothrow_move_constructible<err_t>::value) : m_has_value(p_other.m_has_value)
		{
			if (!m_has_value)
			{
				new (&m_storage.m_error) err_t(std::move(p_other.m_storage.m_error));
			}
			else
			{
				m_storage.m_dummy = 0;
			}
		}

		auto operator=(self_t&& p_other) noexcept(is_nothrow_move_constructible<err_t>::value) -> self_t&
		{
			if (this != &p_other)
			{
				if (!m_has_value && !p_other.m_has_value)
				{
					m_storage.m_error = std::move(p_other.m_storage.m_error);
				}
				else if (m_has_value != p_other.m_has_value)
				{
					this->~expected();
					new (this) self_t(std::move(p_other));
				}
			}
			return *this;
		}

		template <typename err2_t>
		auto operator=(const unexpected<err2_t>& p_unex) -> typename enable_if<std::is_assignable<err_t&, const err2_t&>::value, self_t&>::type
		{
			if (m_has_value)
			{
				new (&m_storage.m_error) err_t(p_unex.error());
				m_has_value = false;
			}
			else
			{
				m_storage.m_error = p_unex.error();
			}
			return *this;
		}

		template <typename err2_t>
		auto operator=(unexpected<err2_t>&& p_unex) -> typename enable_if<std::is_assignable<err_t&, err2_t>::value, self_t&>::type
		{
			if (m_has_value)
			{
				new (&m_storage.m_error) err_t(std::move(p_unex.error()));
				m_has_value = false;
			}
			else
			{
				m_storage.m_error = std::move(p_unex.error());
			}
			return *this;
		}

	public:
		auto emplace() noexcept -> void
		{
			if (!m_has_value)
			{
				m_storage.m_error.~err_t();
				m_has_value = true;
			}
			m_storage.m_dummy = 0;
		}

	public:
		auto has_value() const noexcept -> bool { return m_has_value; }

		explicit operator bool() const noexcept { return m_has_value; }

		auto value() const noexcept -> void {}

		auto operator*() const noexcept -> void {}

		auto error() & noexcept -> err_t& { return m_storage.m_error; }

		auto error() const& noexcept -> const err_t& { return m_storage.m_error; }

		auto error() && noexcept -> err_t&& { return std::move(m_storage.m_error); }

		auto error() const&& noexcept -> const err_t&& { return std::move(m_storage.m_error); }

	public:
		auto swap(self_t& p_other) noexcept -> void
		{
			if (!m_has_value && !p_other.m_has_value)
			{
				using std::swap;
				swap(m_storage.m_error, p_other.m_storage.m_error);
			}
			else if (m_has_value != p_other.m_has_value)
			{
				self_t tmp(std::move(*this));
				*this	= std::move(p_other);
				p_other = std::move(tmp);
			}
		}
	};

	/**
	 * @brief Comparison operators
	 */
	template <typename val1_t, typename err1_t, typename val2_t, typename err2_t>
	auto operator==(const expected<val1_t, err1_t>& p_lhs, const expected<val2_t, err2_t>& p_rhs) -> bool
	{
		if (p_lhs.has_value() != p_rhs.has_value())
		{
			return false;
		}
		if (p_lhs.has_value())
		{
			return *p_lhs == *p_rhs;
		}
		return p_lhs.error() == p_rhs.error();
	}

	template <typename val1_t, typename err1_t, typename val2_t, typename err2_t>
	auto operator!=(const expected<val1_t, err1_t>& p_lhs, const expected<val2_t, err2_t>& p_rhs) -> bool
	{
		return !(p_lhs == p_rhs);
	}

	template <typename val_t, typename err_t, typename val2_t>
	auto operator==(const expected<val_t, err_t>& p_exp, const val2_t& p_val) ->
		typename enable_if<!is_same<val2_t, unexpected<err_t> >::value, bool>::type
	{
		return p_exp.has_value() && (*p_exp == p_val);
	}

	template <typename val_t, typename err_t, typename val2_t>
	auto operator==(const val2_t& p_val, const expected<val_t, err_t>& p_exp) ->
		typename enable_if<!is_same<val2_t, unexpected<err_t> >::value, bool>::type
	{
		return p_exp == p_val;
	}

	template <typename val_t, typename err_t, typename val2_t>
	auto operator!=(const expected<val_t, err_t>& p_exp, const val2_t& p_val) ->
		typename enable_if<!is_same<val2_t, unexpected<err_t> >::value, bool>::type
	{
		return !(p_exp == p_val);
	}

	template <typename val_t, typename err_t, typename val2_t>
	auto operator!=(const val2_t& p_val, const expected<val_t, err_t>& p_exp) ->
		typename enable_if<!is_same<val2_t, unexpected<err_t> >::value, bool>::type
	{
		return !(p_exp == p_val);
	}

	template <typename val_t, typename err_t, typename err2_t>
	auto operator==(const expected<val_t, err_t>& p_exp, const unexpected<err2_t>& p_unex) -> bool
	{
		return !p_exp.has_value() && (p_exp.error() == p_unex.error());
	}

	template <typename val_t, typename err_t, typename err2_t>
	auto operator==(const unexpected<err2_t>& p_unex, const expected<val_t, err_t>& p_exp) -> bool
	{
		return p_exp == p_unex;
	}

	template <typename val_t, typename err_t, typename err2_t>
	auto operator!=(const expected<val_t, err_t>& p_exp, const unexpected<err2_t>& p_unex) -> bool
	{
		return !(p_exp == p_unex);
	}

	template <typename val_t, typename err_t, typename err2_t>
	auto operator!=(const unexpected<err2_t>& p_unex, const expected<val_t, err_t>& p_exp) -> bool
	{
		return !(p_exp == p_unex);
	}

	/**
	 * @brief Swap function
	 */
	template <typename val_t, typename err_t> auto swap(expected<val_t, err_t>& p_lhs, expected<val_t, err_t>& p_rhs) noexcept -> void
	{
		p_lhs.swap(p_rhs);
	}

	template <typename err_t> auto swap(unexpected<err_t>& p_lhs, unexpected<err_t>& p_rhs) noexcept -> void
	{
		p_lhs.swap(p_rhs);
	}

	/**
	 * @brief Helper function to create unexpected value
	 */
	template <typename err_t> auto make_unexpected(err_t&& p_err) -> unexpected<decay_t<err_t> >
	{
		return unexpected<decay_t<err_t> >(std::forward<err_t>(p_err));
	}

}	 // namespace utils

#endif	  // EXPECTED_HPP
