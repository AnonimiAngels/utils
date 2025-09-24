#pragma once
#ifndef TEST_INTERFACE_HPP
	#define TEST_INTERFACE_HPP

	#include "test_utils_arg_parser.hpp"
	#include "test_utils_filesystem.hpp"
	#include "test_utils_process.hpp"

struct test_api
{
	static auto initialize() -> void;

	static auto run_all_tests() -> void
	{
		initialize();
		test_utils_arg_parser::run_all_tests();
		test_utils_filesystem::run_all_tests();
		test_utils_process::run_all_tests();
	}
};

#endif	  // TEST_INTERFACE_HPP
