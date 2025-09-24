#pragma once
#ifndef ARG_PARSER_HPP
	#define ARG_PARSER_HPP

	#include <cctype>
	#include <cstdint>
	#include <cstdlib>
	#include <cstring>
	#include <functional>
	#include <map>
	#include <memory>
	#include <set>
	#include <string>
	#include <type_traits>
	#include <vector>

	#include "cmemory.hpp"
	#include "concepts.hpp"
	#include "cspan.hpp"
	#include "debug.hpp"
	#include "expected.hpp"
	#include "format.hpp"
	#include "loggers.hpp"
	#include "ranges.hpp"
	#include "std/variant.hpp"

namespace utils
{
	enum class arg_type : std::uint8_t
	{
		string_val,
		integer_val,
		boolean_val,
		flag_val,
		float_val
	};

	struct parsed_arg
	{
		std::string name;
		std::string value;
		bool has_value;

		parsed_arg() : has_value(false) {}
	};

	struct arg_info
	{
		std::string long_name;
		std::string short_name;
		std::string description;
		std::string dependency;
		std::string group;
		arg_type type;
		bool required;
		bool was_set;
		bool hidden;

		arg_info() : type(arg_type::string_val), required(false), was_set(false), hidden(false) {}
	};

	template <std::int32_t priority_n> struct priority_tag : priority_tag<priority_n - 1>
	{
	};

	template <> struct priority_tag<0>
	{
	};

	using priority_4 = priority_tag<4>;
	using priority_3 = priority_tag<3>;
	using priority_2 = priority_tag<2>;
	using priority_1 = priority_tag<1>;
	using priority_0 = priority_tag<0>;

	template <typename type_t> struct is_bool_type : std::is_same<decay_t<type_t>, bool>
	{
	};

	template <typename type_t>
	struct is_integer_type : std::integral_constant<bool, std::is_integral<decay_t<type_t> >::value && !is_bool_type<type_t>::value>
	{
	};

	template <typename type_t> struct is_float_type : std::is_floating_point<decay_t<type_t> >
	{
	};

	template <typename type_t>
	struct is_string_type : std::integral_constant<bool,
												   std::is_convertible<type_t, std::string>::value && !is_integer_type<type_t>::value
													   && !is_float_type<type_t>::value && !is_bool_type<type_t>::value>
	{
	};

	class arg_value
	{
	public:
		using self_t	= arg_value;
		using variant_t = std::variant<std::monostate, std::int64_t, double, bool, std::string>;

	private:
		variant_t m_storage;
		arg_type m_type;

	public:
		~arg_value() = default;

		arg_value() : m_storage(std::string{}), m_type(arg_type::string_val) {}

		explicit arg_value(arg_type p_type) : m_type(p_type)
		{
			switch (m_type)
			{
			case arg_type::integer_val:
				m_storage = std::int64_t{0};
				break;
			case arg_type::float_val:
				m_storage = double{0.0};
				break;
			case arg_type::boolean_val:
			case arg_type::flag_val:
				m_storage = bool{false};
				break;
			case arg_type::string_val:
				m_storage = std::string{};
				break;
			}
		}

		arg_value(const self_t& p_other) : m_storage(p_other.m_storage), m_type(p_other.m_type) {}

		arg_value(self_t&& p_other) noexcept : m_storage(std::move(p_other.m_storage)), m_type(p_other.m_type) {}

		auto operator=(const self_t& p_other) -> self_t&
		{
			if (this != &p_other)
			{
				m_storage = p_other.m_storage;
				m_type	  = p_other.m_type;
			}
			return *this;
		}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				m_storage = std::move(p_other.m_storage);
				m_type	  = p_other.m_type;
			}
			return *this;
		}

	public:
		auto set_string(const std::string& p_value) -> void
		{
			m_type	  = arg_type::string_val;
			m_storage = p_value;
		}

		auto set_int(std::int64_t p_value) -> void
		{
			m_type	  = arg_type::integer_val;
			m_storage = p_value;
		}

		auto set_float(double p_value) -> void
		{
			m_type	  = arg_type::float_val;
			m_storage = p_value;
		}

		auto set_bool(bool p_value) -> void
		{
			m_type	  = arg_type::boolean_val;
			m_storage = p_value;
		}

		auto set_flag(bool p_value) -> void
		{
			m_type	  = arg_type::flag_val;
			m_storage = p_value;
		}

		auto get_string() const -> std::string
		{
			if (std::holds_alternative<std::string>(m_storage))
			{
				return std::get<std::string>(m_storage);
			}
			return std::string{};
		}

		auto get_int() const -> std::int64_t
		{
			if (std::holds_alternative<std::int64_t>(m_storage))
			{
				return std::get<std::int64_t>(m_storage);
			}
			return 0;
		}

		auto get_float() const -> double
		{
			if (std::holds_alternative<double>(m_storage))
			{
				return std::get<double>(m_storage);
			}
			return 0.0;
		}

		auto get_bool() const -> bool
		{
			if (std::holds_alternative<bool>(m_storage))
			{
				return std::get<bool>(m_storage);
			}
			return false;
		}

		auto get_type() const -> arg_type { return m_type; }
	};

	class arg_parser
	{
	public:
		using self_t		  = arg_parser;
		using args_vec_t	  = std::vector<std::string>;
		using args_span_t	  = cspan<const char* const>;
		using error_t		  = std::string;
		using result_t		  = expected<bool, error_t>;
		using validation_fn_t = std::function<result_t(const arg_value&)>;
		using binding_fn_t	  = std::function<void(const arg_value&)>;

	private:
		struct arg_entry
		{
			arg_info info;
			arg_value value;
			arg_value default_value;
			validation_fn_t validator;
			binding_fn_t binder;

			arg_entry() : validator(nullptr), binder(nullptr) {}
		};

		using arg_map_t	  = std::map<std::string, std::shared_ptr<arg_entry> >;
		using group_map_t = std::map<std::string, std::vector<std::string> >;

	private:
		std::string m_program_name;
		std::string m_program_path;
		std::string m_version;
		std::string m_description;
		args_vec_t m_input_args;
		arg_map_t m_args;
		group_map_t m_groups;
		std::string m_error_msg;
		bool m_help_requested;
		std::shared_ptr<logger> m_logger;

		// static binding
		static bool m_bool_bind;
		static std::int64_t m_int_bind;
		static double m_float_bind;
		static std::string m_string_bind;

	public:
		~arg_parser() = default;

		arg_parser() : m_version("1.0.0"), m_help_requested(false) {}

		arg_parser(const self_t&)				 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		arg_parser(self_t&& p_other) noexcept
			: m_program_name(std::move(p_other.m_program_name)),
			  m_version(std::move(p_other.m_version)),
			  m_description(std::move(p_other.m_description)),
			  m_input_args(std::move(p_other.m_input_args)),
			  m_args(std::move(p_other.m_args)),
			  m_groups(std::move(p_other.m_groups)),
			  m_error_msg(std::move(p_other.m_error_msg)),
			  m_help_requested(p_other.m_help_requested),
			  m_logger(std::move(p_other.m_logger))
		{
			p_other.m_help_requested = false;
		}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				m_program_name			 = std::move(p_other.m_program_name);
				m_version				 = std::move(p_other.m_version);
				m_description			 = std::move(p_other.m_description);
				m_input_args			 = std::move(p_other.m_input_args);
				m_args					 = std::move(p_other.m_args);
				m_groups				 = std::move(p_other.m_groups);
				m_error_msg				 = std::move(p_other.m_error_msg);
				m_help_requested		 = p_other.m_help_requested;
				m_logger				 = std::move(p_other.m_logger);
				p_other.m_help_requested = false;
			}
			return *this;
		}

	private:
		template <typename type_t> auto detect_arg_type() const -> arg_type
		{
			if (is_bool_type<type_t>::value)
			{
				return arg_type::boolean_val;
			}
			if (is_integer_type<type_t>::value)
			{
				return arg_type::integer_val;
			}
			if (is_float_type<type_t>::value)
			{
				return arg_type::float_val;
			}
			return arg_type::string_val;
		}

		template <typename type_t>
		auto set_value_impl(arg_value& p_val, const type_t& p_value, priority_4) -> typename std::enable_if<is_bool_type<type_t>::value, void>::type
		{
			p_val.set_bool(p_value);
		}

		template <typename type_t>
		auto set_value_impl(arg_value& p_val, const type_t& p_value, priority_3) ->
			typename std::enable_if<is_integer_type<type_t>::value, void>::type
		{
			p_val.set_int(static_cast<std::int64_t>(p_value));
		}

		template <typename type_t>
		auto set_value_impl(arg_value& p_val, const type_t& p_value, priority_2) -> typename std::enable_if<is_float_type<type_t>::value, void>::type
		{
			p_val.set_float(static_cast<double>(p_value));
		}

		template <typename type_t>
		auto set_value_impl(arg_value& p_val, const type_t& p_value, priority_1) -> typename std::enable_if<is_string_type<type_t>::value, void>::type
		{
			p_val.set_string(p_value);
		}

		template <typename type_t> auto set_value_impl(arg_value& p_val, const type_t& p_value) -> void
		{
			set_value_impl<type_t>(p_val, p_value, priority_4{});
		}

		template <typename type_t>
		auto get_value_impl(const arg_value& p_val, priority_4) const -> typename std::enable_if<is_bool_type<type_t>::value, bool>::type
		{
			return p_val.get_bool();
		}

		template <typename type_t>
		auto get_value_impl(const arg_value& p_val, priority_3) const -> typename std::enable_if<is_integer_type<type_t>::value, type_t>::type
		{
			return static_cast<type_t>(p_val.get_int());
		}

		template <typename type_t>
		auto get_value_impl(const arg_value& p_val, priority_2) const -> typename std::enable_if<is_float_type<type_t>::value, type_t>::type
		{
			return static_cast<type_t>(p_val.get_float());
		}

		template <typename type_t>
		auto get_value_impl(const arg_value& p_val, priority_1) const -> typename std::enable_if<is_string_type<type_t>::value, std::string>::type
		{
			return p_val.get_string();
		}

		template <typename type_t> auto get_value_impl(const arg_value& p_val) const -> type_t { return get_value_impl<type_t>(p_val, priority_4{}); }

		template <typename type_t> auto create_binder(type_t& p_bind_value) -> binding_fn_t
		{
			auto* ptr = &p_bind_value;
			return [ptr, this](const arg_value& p_val) { *ptr = get_value_impl<type_t>(p_val); };
		}

		auto find_arg_entry(const std::string& p_name) -> std::shared_ptr<arg_entry>
		{
			auto it = m_args.find(p_name);
			if (it != m_args.end())
			{
				return it->second;
			}

			for (auto& pair : m_args)
			{
				if (pair.second->info.short_name == p_name)
				{
					return pair.second;
				}
			}

			return nullptr;
		}

		auto find_arg_entry(const std::string& p_name) const -> std::shared_ptr<const arg_entry>
		{
			auto it = m_args.find(p_name);
			if (it != m_args.end())
			{
				return it->second;
			}

			for (const auto& pair : m_args)
			{
				if (pair.second->info.short_name == p_name)
				{
					return pair.second;
				}
			}

			return nullptr;
		}

		auto parse_single_arg(const parsed_arg& p_arg, std::size_t& p_idx) -> result_t
		{
			UTILS_DEBUG_LOG("Parsing argument: {} with value: {}", p_arg.name, p_arg.has_value ? p_arg.value : "no value");

			auto entry = find_arg_entry(p_arg.name);
			if (entry == nullptr)
			{
				UTILS_DEBUG_LOG("Unknown argument: {}", p_arg.name);
				return make_unexpected(std::format("Unknown argument: {}", p_arg.name));
			}

			UTILS_DEBUG_LOG("Found argument entry for: {}, type: {}", p_arg.name, static_cast<int>(entry->info.type));
			entry->info.was_set = true;

			if (entry->info.type == arg_type::flag_val)
			{
				entry->value.set_flag(true);
				if (entry->binder)
				{
					entry->binder(entry->value);
				}
				return true;
			}

			std::string value_str;
			if (p_arg.has_value)
			{
				value_str = p_arg.value;
			}
			else if (p_idx + 1 < m_input_args.size())
			{
				const auto& next = m_input_args[p_idx + 1];
				if (!next.empty() && next[0] != '-')
				{
					value_str = next;
					++p_idx;
				}
			}

			if (value_str.empty() && entry->info.type != arg_type::boolean_val)
			{
				return make_unexpected(std::format("Argument {} requires a value", p_arg.name));
			}

			switch (entry->info.type)
			{
			case arg_type::integer_val:
			{
				char* end_ptr = nullptr;
				auto val	  = std::strtoll(value_str.c_str(), &end_ptr, 10);
				if (end_ptr == value_str.c_str() || *end_ptr != '\0')
				{
					return make_unexpected(std::format("Invalid integer value for {}: {}", p_arg.name, value_str));
				}
				entry->value.set_int(val);
				break;
			}
			case arg_type::float_val:
			{
				char* end_ptr = nullptr;
				auto val	  = std::strtod(value_str.c_str(), &end_ptr);
				if (end_ptr == value_str.c_str() || *end_ptr != '\0')
				{
					return make_unexpected(std::format("Invalid float value for {}: {}", p_arg.name, value_str));
				}
				entry->value.set_float(val);
				break;
			}
			case arg_type::boolean_val:
			{
				if (value_str.empty() || value_str == "true" || value_str == "1" || value_str == "yes")
				{
					entry->value.set_bool(true);
				}
				else if (value_str == "false" || value_str == "0" || value_str == "no")
				{
					entry->value.set_bool(false);
				}
				else
				{
					return make_unexpected(std::format("Invalid boolean value for {}: {}", p_arg.name, value_str));
				}
				break;
			}
			case arg_type::string_val:
				entry->value.set_string(value_str);
				break;
			default:
				break;
			}

			if (entry->binder)
			{
				entry->binder(entry->value);
			}

			if (entry->validator != nullptr)
			{
				auto validation_result = entry->validator(entry->value);
				if (!validation_result.has_value())
				{
					return make_unexpected(std::format("Validation failed for {}: {}", p_arg.name, validation_result.error()));
				}
			}

			return true;
		}

		auto validate_args() const -> result_t
		{
			for (const auto& pair : m_args)
			{
				const auto& entry = *pair.second;
				if (entry.info.required && !entry.info.was_set)
				{
					return make_unexpected(
						std::format("Required argument missing: {}", entry.info.long_name.empty() ? entry.info.short_name : entry.info.long_name));
				}
			}

			for (const auto& pair : m_args)
			{
				const auto& entry = *pair.second;
				if (entry.info.was_set && !entry.info.dependency.empty())
				{
					auto dep_entry = find_arg_entry(entry.info.dependency);
					if (dep_entry == nullptr || !dep_entry->info.was_set)
					{
						return make_unexpected(std::format("Argument {} requires {} to be set",
														   entry.info.long_name.empty() ? entry.info.short_name : entry.info.long_name,
														   entry.info.dependency));
					}
				}
			}

			for (const auto& group_pair : m_groups)
			{
				const auto& group_name = group_pair.first;
				const auto& group_args = group_pair.second;

				UTILS_DEBUG_LOG("Validating group: {}", group_name);
				std::size_t set_count = 0;
				for (const auto& arg_name : group_args)
				{
					auto entry = find_arg_entry(arg_name);
					if (entry != nullptr && entry->info.was_set)
					{
						UTILS_DEBUG_LOG("Argument {} in group {} is set", arg_name, group_name);
						++set_count;
					}
					else
					{
						UTILS_DEBUG_LOG("Argument {} in group {} is not set", arg_name, group_name);
					}
				}

				UTILS_DEBUG_LOG("Group {} has {} arguments set", group_name, set_count);
				if (set_count > 1)
				{
					return make_unexpected(std::format("Only one argument from group {} can be set", group_name));
				}
			}

			return true;
		}

		template <typename type_t>
		auto add_arg_impl(const std::string& p_long_name,
						  const std::string& p_short_name,
						  const std::string& p_desc,
						  bool p_required,
						  const type_t& p_default,
						  type_t& p_bind_ptr) -> void
		{
			auto entry				= std::make_shared<arg_entry>();
			entry->info.long_name	= p_long_name;
			entry->info.short_name	= p_short_name;
			entry->info.description = p_desc;
			entry->info.required	= p_required;
			entry->info.type		= detect_arg_type<type_t>();
			entry->info.was_set		= false;
			entry->info.hidden		= false;

			set_value_impl(entry->default_value, p_default);
			entry->value = entry->default_value;

			entry->binder = create_binder<type_t>(p_bind_ptr);

			if (!p_long_name.empty())
			{
				m_args[p_long_name] = entry;
			}
			if (!p_short_name.empty())
			{
				m_args[p_short_name] = entry;
			}
		}

	public:
		template <typename type_t, typename default_t = type_t>
		auto add_arg(const std::string& p_long_name,
					 const std::string& p_desc,
					 const std::string& p_short_name = "",
					 bool p_required				 = false,
					 default_t p_default			 = type_t{}) -> self_t&
		{
			auto entry				= std::make_shared<arg_entry>();
			entry->info.long_name	= p_long_name;
			entry->info.short_name	= p_short_name;
			entry->info.description = p_desc;
			entry->info.required	= p_required;
			entry->info.type		= detect_arg_type<type_t>();
			entry->info.was_set		= false;
			entry->info.hidden		= false;

			set_value_impl(entry->default_value, p_default);
			entry->value = entry->default_value;

			// No binding for 5-parameter version - values retrieved via get_value()
			entry->binder = nullptr;

			if (!p_long_name.empty())
			{
				m_args[p_long_name] = entry;
			}
			if (!p_short_name.empty())
			{
				m_args[p_short_name] = entry;
			}

			return *this;
		}

		template <typename type_t, typename default_t = type_t>
		auto add_arg(const std::string& p_long_name,
					 const std::string& p_desc,
					 const std::string& p_short_name,
					 bool p_required,
					 default_t p_default,
					 default_t& p_bind_value) -> self_t&
		{
			add_arg_impl(p_long_name, p_short_name, p_desc, p_required, p_default, p_bind_value);
			return *this;
		}

		auto
		add_flag(const std::string& p_long_name, const std::string& p_desc, const std::string& p_short_name = "", bool& p_bind_value = m_bool_bind)
			-> self_t&
		{
			auto entry				= std::make_shared<arg_entry>();
			entry->info.long_name	= p_long_name;
			entry->info.short_name	= p_short_name;
			entry->info.description = p_desc;
			entry->info.required	= false;
			entry->info.type		= arg_type::flag_val;
			entry->info.was_set		= false;
			entry->info.hidden		= false;

			entry->value		 = arg_value(arg_type::flag_val);
			entry->default_value = arg_value(arg_type::flag_val);

			entry->binder = create_binder<bool>(p_bind_value);

			if (!p_long_name.empty())
			{
				m_args[p_long_name] = entry;
			}
			if (!p_short_name.empty())
			{
				m_args[p_short_name] = entry;
			}

			return *this;
		}

		template <typename type_t> auto get_value(const std::string& p_name) const -> type_t
		{
			auto entry = find_arg_entry(p_name);
			if (entry == nullptr)
			{
				return type_t{};
			}
			return get_value_impl<type_t>(entry->value);
		}

		auto was_set(const std::string& p_name) const -> bool
		{
			auto entry = find_arg_entry(p_name);
			return entry != nullptr && entry->info.was_set;
		}

		auto parse(std::int32_t p_argc, const char* const* p_argv) -> result_t
		{
			for (std::int32_t i = 0; i < p_argc; ++i)
			{
				m_input_args.emplace_back(p_argv[i]);
			}

			return parse(m_input_args);
		}

		auto parse(const args_vec_t& p_args) -> result_t
		{
			UTILS_DEBUG_LOG("Starting parse with {} arguments", p_args.size());

			for (auto& pair : m_args)
			{
				auto& entry			= pair.second;
				entry->info.was_set = false;
				entry->value		= entry->default_value;

				// Call binder with default value if binder exists
				if (entry->binder)
				{
					entry->binder(entry->default_value);
				}
			}

			if (p_args.size() > 0)
			{
				const auto& first_arg = p_args.begin();

				m_program_name = first_arg->substr(p_args[0].find_last_of("/\\") + 1);
				m_program_path = first_arg->substr(0, p_args[0].find_last_of("/\\") + 1);

				if (m_program_name.empty())
				{
					m_program_name = "program";
				}

				if (m_program_path.empty())
				{
					m_program_path = ".";
				}
			}

			m_input_args = p_args;
			m_input_args.erase(m_input_args.begin());

			for (std::size_t idx_for = 0; idx_for < m_input_args.size(); ++idx_for)
			{
				const auto& arg = m_input_args[idx_for];

				if (arg == "-h" || arg == "--help")
				{
					m_help_requested = true;
					continue;
				}

				parsed_arg p_arg;
				if (!arg.empty())
				{
					auto eq_pos = arg.find('=');
					if (eq_pos != std::string::npos)
					{
						p_arg.name		= arg.substr(0, eq_pos);
						p_arg.value		= arg.substr(eq_pos + 1);
						p_arg.has_value = true;
					}
					else
					{
						p_arg.name		= arg;
						p_arg.has_value = false;
					}

					auto result = parse_single_arg(p_arg, idx_for);
					if (!result.has_value())
					{
						m_error_msg = result.error();
						return result;
					}
				}
			}

			auto validation_result = validate_args();
			if (!validation_result.has_value())
			{
				m_error_msg = validation_result.error();
				return validation_result;
			}

			return true;
		}

	public:
		auto add_validator(const std::string& p_arg_name, validation_fn_t p_validator) -> self_t&
		{
			auto entry = find_arg_entry(p_arg_name);
			if (entry != nullptr)
			{
				entry->validator = p_validator;
			}
			return *this;
		}

		auto add_dependency(const std::string& p_arg_name, const std::string& p_depends_on) -> self_t&
		{
			auto entry = find_arg_entry(p_arg_name);
			if (entry != nullptr)
			{
				entry->info.dependency = p_depends_on;
			}
			return *this;
		}

		auto add_group(const std::string& p_group_name, const std::vector<std::string>& p_args) -> self_t&
		{
			m_groups[p_group_name] = p_args;
			for (const auto& arg_name : p_args)
			{
				auto entry = find_arg_entry(arg_name);
				if (entry != nullptr)
				{
					entry->info.group = p_group_name;
				}
			}
			return *this;
		}

		auto set_hidden(const std::string& p_arg_name, bool p_hidden = true) -> self_t&
		{
			auto entry = find_arg_entry(p_arg_name);
			if (entry != nullptr)
			{
				entry->info.hidden = p_hidden;
			}
			return *this;
		}

		auto generate_help() const -> std::string
		{
			std::string help_text;
			help_text += std::format("{} v{}\n", m_program_name, m_version);
			if (!m_description.empty())
			{
				help_text += std::format("{}\n", m_description);
			}
			help_text += "\nUsage: " + m_program_name + " [options]\n\n";
			help_text += "Options:\n";
			help_text += "  -h, --help      Show this help message\n";

			std::vector<std::shared_ptr<const arg_entry> > visible_args;
			std::set<std::string> processed;

			for (const auto& pair : m_args)
			{
				const auto& entry = *pair.second;
				if (!entry.info.hidden)
				{
					auto key = entry.info.long_name.empty() ? entry.info.short_name : entry.info.long_name;
					if (processed.find(key) == processed.end())
					{
						visible_args.push_back(pair.second);
						processed.insert(key);
					}
				}
			}

			for (const auto& entry_ptr : visible_args)
			{
				help_text += format_arg_help(*entry_ptr);
			}

			return help_text;
		}

	private:
		auto format_arg_help(const arg_entry& p_entry) const -> std::string
		{
			std::string line = "  ";

			if (!p_entry.info.short_name.empty())
			{
				line += p_entry.info.short_name;
				if (!p_entry.info.long_name.empty())
				{
					line += ", ";
				}
			}
			else
			{
				line += "    ";
			}

			if (!p_entry.info.long_name.empty())
			{
				line += p_entry.info.long_name;
			}

			switch (p_entry.info.type)
			{
			case arg_type::string_val:
				line += " <string>";
				break;
			case arg_type::integer_val:
				line += " <int>";
				break;
			case arg_type::float_val:
				line += " <float>";
				break;
			case arg_type::boolean_val:
				line += " <bool>";
				break;
			case arg_type::flag_val:
				break;
			}

			line += "\n        ";
			line += p_entry.info.description;

			if (p_entry.info.required)
			{
				line += " (required)";
			}
			if (!p_entry.info.dependency.empty())
			{
				line += std::format(" (depends on {})", p_entry.info.dependency);
			}

			return line + "\n";
		}

		auto get_error_msg() const -> std::string { return m_error_msg; }

	public:
		auto set_version(const std::string& p_version) -> void { m_version = p_version; }

		auto get_version() const -> std::string { return m_version; }

		auto set_description(const std::string& p_desc) -> void { m_description = p_desc; }

		auto get_description() const -> std::string { return m_description; }

		auto get_program_name() const -> std::string { return m_program_name; }

		auto is_help_requested() const -> bool { return m_help_requested; }

		auto get_all_args() const -> std::vector<std::string>
		{
			std::vector<std::string> result;
			for (const auto& pair : m_args)
			{
				const auto& entry = *pair.second;
				if (!entry.info.long_name.empty())
				{
					result.push_back(entry.info.long_name);
				}
				else if (!entry.info.short_name.empty())
				{
					result.push_back(entry.info.short_name);
				}
			}
			return result;
		}

		auto get_set_args() const -> std::vector<std::string>
		{
			std::vector<std::string> result;
			for (const auto& pair : m_args)
			{
				const auto& entry = *pair.second;
				if (entry.info.was_set)
				{
					if (!entry.info.long_name.empty())
					{
						result.push_back(entry.info.long_name);
					}
					else if (!entry.info.short_name.empty())
					{
						result.push_back(entry.info.short_name);
					}
				}
			}
			return result;
		}

		auto set_logger(std::shared_ptr<logger> p_logger) -> void { m_logger = p_logger; }
	};

}	 // namespace utils

#endif	  // ARG_PARSER_HPP
