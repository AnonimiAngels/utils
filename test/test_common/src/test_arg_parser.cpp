#include "test_utils_arg_parser.hpp"

namespace test_utils_arg_parser
{
	auto test_basic_string_argument(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Basic String Argument", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--name", "Name argument", "-n");

			std::vector<std::string> args = {"program", "--name", "test_value"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_true(parser.was_set("--name"));
			p_runner.assert_equals(parser.get_value<std::string>("--name"), std::string("test_value"));
		});
	}

	auto test_basic_integer_argument(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Basic Integer Argument", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::int32_t>("--count", "Count argument", "-c");

			std::vector<std::string> args = {"program", "--count", "42"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_true(parser.was_set("--count"));
			p_runner.assert_equals(parser.get_value<std::int32_t>("--count"), 42);
		});
	}

	auto test_basic_double_argument(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Basic Double Argument", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<double>("--rate", "Rate argument", "-r");

			std::vector<std::string> args = {"program", "--rate", "3.14"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_true(parser.was_set("--rate"));
			p_runner.assert_equals(parser.get_value<double>("--rate"), 3.14);
		});
	}

	auto test_basic_bool_argument(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Basic Bool Argument", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<bool>("--enable", "Enable flag", "-e");

			std::vector<std::string> args = {"program", "--enable", "true"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_true(parser.was_set("--enable"));
			p_runner.assert_true(parser.get_value<bool>("--enable"));
		});
	}

	auto test_boolean_values(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Boolean Values", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<bool>("--flag1", "Flag 1", "");
			parser.add_arg<bool>("--flag2", "Flag 2", "");
			parser.add_arg<bool>("--flag3", "Flag 3", "");
			parser.add_arg<bool>("--flag4", "Flag 4", "");
			parser.add_arg<bool>("--flag5", "Flag 5", "");

			std::vector<std::string> args = {"program", "--flag1", "false", "--flag2", "0", "--flag3", "no", "--flag4", "1", "--flag5", "yes"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_false(parser.get_value<bool>("--flag1"));
			p_runner.assert_false(parser.get_value<bool>("--flag2"));
			p_runner.assert_false(parser.get_value<bool>("--flag3"));
			p_runner.assert_true(parser.get_value<bool>("--flag4"));
			p_runner.assert_true(parser.get_value<bool>("--flag5"));
		});
	}

	auto test_flag_argument(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Flag Argument", [&]()
		{
			utils::arg_parser parser;
			bool verbose = false;
			parser.add_flag("--verbose", "Verbose output", "-v", verbose);

			std::vector<std::string> args = {"program", "--verbose"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_true(parser.was_set("--verbose"));
			p_runner.assert_true(verbose);
		});
	}

	auto test_short_argument_names(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Short Argument Names", [&]()
		{
			utils::arg_parser parser;
			bool verbose = false;

			parser.add_arg<std::string>("--name", "Name", "-n");
			parser.add_arg<std::int32_t>("--count", "Count", "-c");
			parser.add_flag("--verbose", "Verbose", "-v", verbose);

			std::vector<std::string> args = {"program", "-n", "test", "-c", "5", "-v"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_equals(parser.get_value<std::string>("--name"), std::string("test"));
			p_runner.assert_equals(parser.get_value<std::int32_t>("--count"), 5);
			p_runner.assert_true(verbose);
		});
	}

	auto test_equals_syntax(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Equals Syntax", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--name", "Name", "");
			parser.add_arg<std::int32_t>("--port", "Port", "");

			std::vector<std::string> args = {"program", "--name=server", "--port=8080"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_equals(parser.get_value<std::string>("--name"), std::string("server"));
			p_runner.assert_equals(parser.get_value<std::int32_t>("--port"), 8080);
		});
	}

	auto test_default_values(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Default Values", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--name", "Name", "", false, std::string("default_name"));
			parser.add_arg<std::int32_t>("--count", "Count", "", false, 42);
			parser.add_arg<double>("--rate", "Rate", "", false, 1.5);
			parser.add_arg<bool>("--enable", "Enable", "", false, true);

			std::vector<std::string> args = {"program"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_equals(parser.get_value<std::string>("--name"), std::string("default_name"));
			p_runner.assert_equals(parser.get_value<std::int32_t>("--count"), 42);
			p_runner.assert_equals(parser.get_value<double>("--rate"), 1.5);
			p_runner.assert_true(parser.get_value<bool>("--enable"));
		});
	}

	auto test_required_arguments(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Required Arguments", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--name", "Required name", "", true);

			std::vector<std::string> args = {"program"};
			auto result = parser.parse(args);

			p_runner.assert_false(result.has_value());
			std::string error_str = result.error();
			p_runner.assert_true(error_str.find("Required argument missing") != std::string::npos);
		});
	}

	auto test_required_arguments_satisfied(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Required Arguments Satisfied", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--name", "Required name", "", true);

			std::vector<std::string> args = {"program", "--name", "provided"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_equals(parser.get_value<std::string>("--name"), std::string("provided"));
		});
	}

	auto test_invalid_integer(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Invalid Integer", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::int32_t>("--count", "Count", "");

			std::vector<std::string> args = {"program", "--count", "not_a_number"};
			auto result = parser.parse(args);

			p_runner.assert_false(result.has_value());
			std::string error_str = result.error();
			p_runner.assert_true(error_str.find("Invalid integer value") != std::string::npos);
		});
	}

	auto test_invalid_float(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Invalid Float", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<double>("--rate", "Rate", "");

			std::vector<std::string> args = {"program", "--rate", "not_a_float"};
			auto result = parser.parse(args);

			p_runner.assert_false(result.has_value());
			std::string error_str = result.error();
			p_runner.assert_true(error_str.find("Invalid float value") != std::string::npos);
		});
	}

	auto test_invalid_boolean(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Invalid Boolean", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<bool>("--enable", "Enable", "");

			std::vector<std::string> args = {"program", "--enable", "maybe"};
			auto result = parser.parse(args);

			p_runner.assert_false(result.has_value());
			std::string error_str = result.error();
			p_runner.assert_true(error_str.find("Invalid boolean value") != std::string::npos);
		});
	}

	auto test_unknown_argument(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Unknown Argument", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--name", "Name", "");

			std::vector<std::string> args = {"program", "--unknown", "value"};
			auto result = parser.parse(args);

			p_runner.assert_false(result.has_value());
			std::string error_str = result.error();
			p_runner.assert_true(error_str.find("Unknown argument") != std::string::npos);
		});
	}

	auto test_argument_validation(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Argument Validation", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::int32_t>("--port", "Port number", "", false, 8080);

			parser.add_validator("--port",
								 [](const utils::arg_value& val) -> utils::arg_parser::result_t
								 {
									 std::int32_t port_val = val.get_int();
									 if (port_val < 1 || port_val > 65535)
									 {
										 return utils::make_unexpected(std::string("Port must be between 1 and 65535"));
									 }
									 return true;
								 });

			std::vector<std::string> args1 = {"program", "--port", "70000"};
			auto result1 = parser.parse(args1);
			p_runner.assert_false(result1.has_value());
			std::string error_str1 = result1.error();
			p_runner.assert_true(error_str1.find("Port must be between 1 and 65535") != std::string::npos);

			std::vector<std::string> args2 = {"program", "--port", "8080"};
			auto result2 = parser.parse(args2);
			p_runner.assert_true(result2.has_value());
			p_runner.assert_equals(parser.get_value<std::int32_t>("--port"), 8080);
		});
	}

	auto test_argument_dependencies(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Argument Dependencies", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--username", "Username", "-u");
			parser.add_arg<std::string>("--password", "Password", "-p");
			parser.add_dependency("--password", "--username");

			std::vector<std::string> args1 = {"program", "--password", "secret"};
			auto result1 = parser.parse(args1);
			p_runner.assert_false(result1.has_value());
			std::string error_str1 = result1.error();
			p_runner.assert_true(error_str1.find("requires --username to be set") != std::string::npos);

			std::vector<std::string> args2 = {"program", "--username", "user", "--password", "secret"};
			auto result2 = parser.parse(args2);
			p_runner.assert_true(result2.has_value());
			p_runner.assert_equals(parser.get_value<std::string>("--username"), std::string("user"));
			p_runner.assert_equals(parser.get_value<std::string>("--password"), std::string("secret"));
		});
	}

	auto test_argument_groups(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Argument Groups", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--client", "Client mode", "");
			parser.add_arg<std::string>("--server", "Server mode", "");
			parser.add_group("mode", {"--client", "--server"});

			std::vector<std::string> args1 = {"program", "--client", "config1", "--server", "config2"};
			auto result1 = parser.parse(args1);
			p_runner.assert_false(result1.has_value());
			std::string error_str1 = result1.error();
			p_runner.assert_true(error_str1.find("Only one argument from group mode can be set") != std::string::npos);

			std::vector<std::string> args2 = {"program", "--client", "config1"};
			auto result2 = parser.parse(args2);
			p_runner.assert_true(result2.has_value());
			p_runner.assert_equals(parser.get_value<std::string>("--client"), std::string("config1"));
		});
	}

	auto test_help_request(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Help Request", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--name", "Your name", "-n");

			std::vector<std::string> args = {"program", "--help"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_true(parser.is_help_requested());
		});
	}

	auto test_help_request_short(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Help Request Short", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--name", "Your name", "-n");

			std::vector<std::string> args = {"program", "-h"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_true(parser.is_help_requested());
		});
	}

	auto test_help_generation(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Help Generation", [&]()
		{
			utils::arg_parser parser;
			parser.set_version("2.0.0");
			parser.set_description("Test application for arg parser");

			parser.add_arg<std::string>("--name", "Your name", "-n", true);
			parser.add_arg<std::int32_t>("--count", "Number of items", "-c", false, 10);
			bool verbose = false;
			parser.add_flag("--verbose", "Enable verbose output", "-v", verbose);

			std::string help = parser.generate_help();

			p_runner.assert_true(help.find("2.0.0") != std::string::npos);
			p_runner.assert_true(help.find("Test application for arg parser") != std::string::npos);
			p_runner.assert_true(help.find("--name") != std::string::npos);
			p_runner.assert_true(help.find("Your name") != std::string::npos);
			p_runner.assert_true(help.find("(required)") != std::string::npos);
			p_runner.assert_true(help.find("--count") != std::string::npos);
			p_runner.assert_true(help.find("--verbose") != std::string::npos);
			p_runner.assert_true(help.find("-h, --help") != std::string::npos);
		});
	}

	auto test_hidden_arguments(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Hidden Arguments", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--debug", "Debug option", "");
			parser.set_hidden("--debug", true);

			std::string help = parser.generate_help();
			p_runner.assert_false(help.find("--debug") != std::string::npos);

			std::vector<std::string> args = {"program", "--debug", "test"};
			auto result = parser.parse(args);
			p_runner.assert_true(result.has_value());
			p_runner.assert_equals(parser.get_value<std::string>("--debug"), std::string("test"));
		});
	}

	auto test_get_all_args(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Get All Args", [&]()
		{
			utils::arg_parser parser;
			bool verbose = false;
			parser.add_arg<std::string>("--name", "Name", "-n");
			parser.add_arg<std::int32_t>("--count", "Count", "-c");
			parser.add_flag("--verbose", "Verbose", "-v", verbose);

			auto all_args = parser.get_all_args();
			p_runner.assert_true(std::find(all_args.begin(), all_args.end(), "--name") != all_args.end());
			p_runner.assert_true(std::find(all_args.begin(), all_args.end(), "--count") != all_args.end());
			p_runner.assert_true(std::find(all_args.begin(), all_args.end(), "--verbose") != all_args.end());
		});
	}

	auto test_get_set_args(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Get Set Args", [&]()
		{
			utils::arg_parser parser;
			bool verbose = false;
			parser.add_arg<std::string>("--name", "Name", "-n");
			parser.add_arg<std::int32_t>("--count", "Count", "-c");
			parser.add_flag("--verbose", "Verbose", "-v", verbose);

			std::vector<std::string> args = {"program", "--name", "test", "--verbose"};
			auto result = parser.parse(args);
			p_runner.assert_true(result.has_value());

			auto set_args = parser.get_set_args();
			p_runner.assert_true(std::find(set_args.begin(), set_args.end(), "--name") != set_args.end());
			p_runner.assert_true(std::find(set_args.begin(), set_args.end(), "--verbose") != set_args.end());
			p_runner.assert_false(std::find(set_args.begin(), set_args.end(), "--count") != set_args.end());
		});
	}

	auto test_missing_value_for_argument(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Missing Value for Argument", [&]()
		{
			utils::arg_parser parser;
			parser.add_arg<std::string>("--name", "Name", "");

			std::vector<std::string> args = {"program", "--name"};
			auto result = parser.parse(args);

			p_runner.assert_false(result.has_value());
			std::string error_str = result.error();
			p_runner.assert_true(error_str.find("requires a value") != std::string::npos);
		});
	}

	auto test_binding_with_6_parameter_version(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("Binding with 6-Parameter Version", [&]()
		{
			utils::arg_parser parser;
			std::string name = "initial";
			std::string name_default = "default";
			std::int32_t count = 0;
			std::int32_t count_default = 42;
			bool enable = false;
			bool enable_default = true;

			parser.add_arg<std::string>("--name", "Name", "-n", false, name_default, name);
			parser.add_arg<std::int32_t>("--count", "Count", "-c", false, count_default, count);
			parser.add_arg<bool>("--enable", "Enable", "-e", false, enable_default, enable);

			std::vector<std::string> args = {"program", "--name", "bound_value", "--count", "100"};
			auto result = parser.parse(args);

			p_runner.assert_true(result.has_value());
			p_runner.assert_equals(name, std::string("bound_value"));
			p_runner.assert_equals(count, 100);
			p_runner.assert_true(enable); // Should use default since not set
		});
	}

	auto run_all_tests() -> void
	{
		auto runner = test_common::create_test_runner("Argument Parser Tests", true);

		test_basic_string_argument(*runner);
		test_basic_integer_argument(*runner);
		test_basic_double_argument(*runner);
		test_basic_bool_argument(*runner);
		test_boolean_values(*runner);
		test_flag_argument(*runner);
		test_short_argument_names(*runner);
		test_equals_syntax(*runner);
		test_default_values(*runner);
		test_required_arguments(*runner);
		test_required_arguments_satisfied(*runner);
		test_invalid_integer(*runner);
		test_invalid_float(*runner);
		test_invalid_boolean(*runner);
		test_unknown_argument(*runner);
		test_argument_validation(*runner);
		test_argument_dependencies(*runner);
		test_argument_groups(*runner);
		test_help_request(*runner);
		test_help_request_short(*runner);
		test_help_generation(*runner);
		test_hidden_arguments(*runner);
		test_get_all_args(*runner);
		test_get_set_args(*runner);
		test_missing_value_for_argument(*runner);
		test_binding_with_6_parameter_version(*runner);
	}
}	 // namespace test_utils_arg_parser