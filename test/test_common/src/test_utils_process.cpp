#include "test_utils_process.hpp"

#include <chrono>
#include <thread>

namespace test_utils_process
{
	auto run_basic_process_tests(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("default_construction",
							   [&]()
							   {
								   process p;
								   p_runner.assert_equals(p.get_return_code(), 0);
								   p_runner.assert_true(p.get_output().empty());
							   });

		p_runner.run_test_case("constructor_with_command",
							   [&]()
							   {
								   process p("echo hello world");
								   p_runner.assert_equals(p.get_return_code(), 0);
								   p_runner.assert_equals(p.get_output(), std::string("hello world"));
							   });

		p_runner.run_test_case("move_constructor",
							   [&]()
							   {
								   process p1("echo test move");
								   std::string expected_output = p1.get_output();
								   std::int32_t expected_return_code = p1.get_return_code();

								   process p2 = std::move(p1);
								   p_runner.assert_equals(p2.get_output(), expected_output);
								   p_runner.assert_equals(p2.get_return_code(), expected_return_code);
							   });
	}

	auto run_sync_execution_tests(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("simple_echo_command",
							   [&]()
							   {
								   process p;
								   p.execute("echo sync test");
								   p_runner.assert_equals(p.get_return_code(), 0);
								   p_runner.assert_equals(p.get_output(), std::string("sync test"));
							   });

		p_runner.run_test_case("command_with_exit_code",
							   [&]()
							   {
								   process p;
								   p.execute("exit 42");
								   p_runner.assert_equals(p.get_return_code(), 42);
							   });

		p_runner.run_test_case("multiline_output",
							   [&]()
							   {
								   process p;
								   p.execute("printf line1\\\\nline2\\\\nline3");
								   p_runner.assert_equals(p.get_output(), std::string("line1\nline2\nline3"));
							   });

		p_runner.run_test_case("command_with_arguments",
							   [&]()
							   {
								   process p;
								   p.execute("ls /");
								   p_runner.assert_equals(p.get_return_code(), 0);
								   p_runner.assert_false(p.get_output().empty());
							   });

		p_runner.run_test_case("sequential_executions",
							   [&]()
							   {
								   process p;

								   p.execute("echo first");
								   p_runner.assert_equals(p.get_output(), std::string("first"));
								   p_runner.assert_equals(p.get_return_code(), 0);

								   p.execute("echo second");
								   p_runner.assert_equals(p.get_output(), std::string("second"));
								   p_runner.assert_equals(p.get_return_code(), 0);
							   });
	}

	auto run_async_execution_tests(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("async_execution",
							   [&]()
							   {
								   process p;
								   p.execute("echo async test", true);
								   p.wait();
								   p_runner.assert_equals(p.get_return_code(), 0);
								   p_runner.assert_equals(p.get_output(), std::string("async test"));
							   });

		p_runner.run_test_case("async_with_delay",
							   [&]()
							   {
								   process p;
								   auto start_time = std::chrono::steady_clock::now();
								   p.execute("sleep 0.1 && echo delayed", true);

								   // Should return immediately for async execution
								   auto immediate_time = std::chrono::steady_clock::now();
								   auto immediate_duration = std::chrono::duration_cast<std::chrono::milliseconds>(immediate_time - start_time);
								   p_runner.assert_true(immediate_duration.count() < 50, "Async execution should return immediately");

								   // Wait for completion
								   p.wait();
								   auto complete_time = std::chrono::steady_clock::now();
								   auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(complete_time - start_time);
								   p_runner.assert_true(total_duration.count() >= 100, "Should have waited for command completion");

								   p_runner.assert_equals(p.get_return_code(), 0);
								   p_runner.assert_equals(p.get_output(), std::string("delayed"));
							   });

		p_runner.run_test_case("multiple_async_executions",
							   [&]()
							   {
								   process p1;
								   process p2;

								   p1.execute("echo first async", true);
								   p2.execute("echo second async", true);

								   p1.wait();
								   p2.wait();

								   p_runner.assert_equals(p1.get_return_code(), 0);
								   p_runner.assert_equals(p1.get_output(), std::string("first async"));
								   p_runner.assert_equals(p2.get_return_code(), 0);
								   p_runner.assert_equals(p2.get_output(), std::string("second async"));
							   });
	}

	auto run_error_handling_tests(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("invalid_command",
							   [&]()
							   {
								   process p;
								   p.execute("nonexistent_command_12345");
								   p_runner.assert_true(p.get_return_code() != 0);
								   p_runner.assert_false(p.get_output().empty());
							   });

		p_runner.run_test_case("command_sanitization",
							   [&]()
							   {
								   process p;
								   p.execute("echo test\necho should not execute\r");
								   // Newlines and carriage returns are replaced with spaces
								   p_runner.assert_equals(p.get_return_code(), 0);
								   p_runner.assert_equals(p.get_output(), std::string("test echo should not execute"));
							   });

		p_runner.run_test_case("command_failure",
							   [&]()
							   {
								   process p;
								   p.execute("false");  // Command that always fails
								   p_runner.assert_true(p.get_return_code() != 0);
							   });

		p_runner.run_test_case("empty_command",
							   [&]()
							   {
								   process p;
								   p.execute("");
								   p_runner.assert_equals(p.get_return_code(), 0);
								   p_runner.assert_true(p.get_output().empty());
							   });

		p_runner.run_test_case("async_error_handling",
							   [&]()
							   {
								   process p;
								   p.execute("nonexistent_command_async_12345", true);
								   p.wait();
								   p_runner.assert_true(p.get_return_code() != 0);
								   p_runner.assert_false(p.get_output().empty());
							   });
	}
}