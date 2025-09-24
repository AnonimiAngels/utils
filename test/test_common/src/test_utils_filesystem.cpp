#include "test_utils_filesystem.hpp"

namespace test_utils_filesystem
{
	auto run_basic_filesystem_tests(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("fs_namespace_available",
							   [&]()
							   {
								   fs::path test_path("test");
								   p_runner.assert_true(true);
							   });

		p_runner.run_test_case("path_construction",
							   [&]()
							   {
								   fs::path p1("test.txt");
								   fs::path p2("/home/user/test.txt");
								   fs::path p3;

								   p_runner.assert_false(p1.empty());
								   p_runner.assert_false(p2.empty());
								   p_runner.assert_true(p3.empty());
							   });

		p_runner.run_test_case("path_operations",
							   [&]()
							   {
								   fs::path p("dir/subdir/file.txt");

								   p_runner.assert_equals(p.filename().string(), std::string("file.txt"));
								   p_runner.assert_equals(p.extension().string(), std::string(".txt"));
								   p_runner.assert_equals(p.stem().string(), std::string("file"));
								   p_runner.assert_equals(p.parent_path().string(), std::string("dir/subdir"));
							   });

		p_runner.run_test_case("path_concatenation",
							   [&]()
							   {
								   fs::path base("dir");
								   fs::path sub("subdir");
								   fs::path file("file.txt");

								   fs::path combined = base / sub / file;
								   p_runner.assert_equals(combined.string(), std::string("dir/subdir/file.txt"));
							   });
	}

	auto run_file_operations_tests(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("file_exists",
							   [&]()
							   {
								   helpers::temp_path temp_file = helpers::create_test_file("test_exists.txt");

								   p_runner.assert_true(fs::exists(temp_file));

								   fs::path non_existent("non_existent_file_12345.txt");
								   p_runner.assert_false(fs::exists(non_existent));
							   });

		p_runner.run_test_case("directory_operations",
							   [&]()
							   {
								   helpers::temp_path temp_dir = helpers::create_test_directory("test_dir_ops");

								   p_runner.assert_true(fs::exists(temp_dir));
								   p_runner.assert_true(fs::is_directory(temp_dir));

								   fs::path subdir = temp_dir.path() / "subdir";
								   fs::create_directory(subdir);
								   p_runner.assert_true(fs::exists(subdir));
								   p_runner.assert_true(fs::is_directory(subdir));
							   });

		p_runner.run_test_case("file_size",
							   [&]()
							   {
								   std::string content			= "Hello, World!";
								   helpers::temp_path temp_file = helpers::create_test_file("test_size.txt", content);

								   p_runner.assert_true(fs::exists(temp_file));
								   p_runner.assert_equals(fs::file_size(temp_file), content.size());
							   });

		p_runner.run_test_case("file_type_checks",
							   [&]()
							   {
								   helpers::temp_path temp_file = helpers::create_test_file("test_type.txt");
								   helpers::temp_path temp_dir	= helpers::create_test_directory("test_type_dir");

								   p_runner.assert_true(fs::is_regular_file(temp_file));
								   p_runner.assert_false(fs::is_directory(temp_file));

								   p_runner.assert_true(fs::is_directory(temp_dir));
								   p_runner.assert_false(fs::is_regular_file(temp_dir));
							   });
	}

	auto run_path_manipulation_tests(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("absolute_relative_paths",
							   [&]()
							   {
								   fs::path relative("dir/file.txt");
								   fs::path absolute = fs::absolute(relative);

								   p_runner.assert_false(relative.is_absolute());
								   p_runner.assert_true(absolute.is_absolute());
							   });

		p_runner.run_test_case("canonical_path",
							   [&]()
							   {
								   helpers::temp_path temp_dir	= helpers::create_test_directory("test_canonical");
								   helpers::temp_path temp_file = helpers::create_test_file("test_canonical.txt");

								   fs::path target = temp_dir.path() / "canonical_file.txt";
								   fs::copy_file(temp_file, target);

								   fs::path complex_path = temp_dir.path() / ".." / temp_dir.path().filename() / "canonical_file.txt";
								   fs::path canonical	 = fs::canonical(complex_path);

								   p_runner.assert_true(fs::exists(canonical));
								   p_runner.assert_equals(canonical, target);
							   });

		p_runner.run_test_case("relative_path_computation",
							   [&]()
							   {
								   helpers::temp_path temp_dir = helpers::create_test_directory("test_relative");
								   fs::path subdir			   = temp_dir.path() / "subdir";
								   fs::create_directory(subdir);

								   fs::path file_path = subdir / "file.txt";
								   std::ofstream(file_path) << "test";

								   fs::path relative = fs::relative(file_path, temp_dir);
								   p_runner.assert_equals(relative.string(), std::string("subdir/file.txt"));
							   });
	}

	auto run_iterator_tests(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("directory_iteration",
							   [&]()
							   {
								   helpers::temp_path temp_dir = helpers::create_test_directory("test_iteration");

								   std::ofstream(temp_dir.path() / "file1.txt") << "content1";
								   std::ofstream(temp_dir.path() / "file2.txt") << "content2";
								   std::ofstream(temp_dir.path() / "file3.txt") << "content3";

								   std::int32_t file_count = 0;
								   for (const auto& entry : fs::directory_iterator(temp_dir))
								   {
									   p_runner.assert_true(fs::is_regular_file(entry));
									   file_count++;
								   }

								   p_runner.assert_equals(file_count, 3);
							   });

		p_runner.run_test_case("recursive_directory_iteration",
							   [&]()
							   {
								   helpers::temp_path temp_dir = helpers::create_test_directory("test_recursive");

								   fs::path subdir = temp_dir.path() / "subdir";
								   fs::create_directory(subdir);

								   std::ofstream(temp_dir.path() / "root_file.txt") << "root";
								   std::ofstream(subdir / "sub_file.txt") << "sub";

								   std::int32_t total_files = 0;
								   for (const auto& entry : fs::recursive_directory_iterator(temp_dir))
								   {
									   if (fs::is_regular_file(entry))
									   {
										   total_files++;
									   }
								   }

								   p_runner.assert_equals(total_files, 2);
							   });
	}

	auto run_error_handling_tests(test_common::test_runner& p_runner) -> void
	{
		p_runner.run_test_case("error_code_handling",
							   [&]()
							   {
								   fs::path non_existent("non_existent_path_12345");
								   std::error_code ec;

								   bool exists = fs::exists(non_existent, ec);
								   p_runner.assert_false(exists);

								   auto size = fs::file_size(non_existent, ec);
								   p_runner.assert_true(static_cast<bool>(ec));
								   p_runner.assert_equals(size, static_cast<std::uintmax_t>(-1));
							   });

		p_runner.run_test_case("exception_handling",
							   [&]()
							   {
								   fs::path non_existent("non_existent_path_12345");

								   // Test that the exception is thrown
								   bool exception_thrown = false;
								   try
								   {
									   fs::file_size(non_existent);
								   }
								   catch (const fs::filesystem_error&)
								   {
									   exception_thrown = true;
								   }
								   p_runner.assert_true(exception_thrown, "Expected filesystem_error to be thrown");
							   });
	}

	auto run_all_tests() -> void
	{
		auto runner = test_common::create_test_runner("Utils Filesystem Tests", true);

		run_basic_filesystem_tests(*runner);
		run_file_operations_tests(*runner);
		run_path_manipulation_tests(*runner);
		run_iterator_tests(*runner);
		run_error_handling_tests(*runner);
	}
}	 // namespace test_utils_filesystem