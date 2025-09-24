#pragma once
#ifndef TEST_UTILS_PROCESS_HPP
	#define TEST_UTILS_PROCESS_HPP

	#include "test_common.hpp"
	#include "utils_process.hpp"

namespace test_utils_process
{
	auto run_basic_process_tests(test_common::test_runner& p_runner) -> void;
	auto run_sync_execution_tests(test_common::test_runner& p_runner) -> void;
	auto run_async_execution_tests(test_common::test_runner& p_runner) -> void;
	auto run_error_handling_tests(test_common::test_runner& p_runner) -> void;

	static inline auto run_all_tests() -> void
	{
		test_common::test_runner runner("test_utils_process", false);
		run_basic_process_tests(runner);
		run_sync_execution_tests(runner);
		run_async_execution_tests(runner);
		run_error_handling_tests(runner);
	}
}

#endif // TEST_UTILS_PROCESS_HPP