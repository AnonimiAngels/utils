#pragma once
#ifndef TEST_UTILS_FILESYSTEM_HPP
	#define TEST_UTILS_FILESYSTEM_HPP

	#include <fstream>
	#include <string>

	#include "filesystem.hpp"
	#include "test_common.hpp"

namespace test_utils_filesystem
{

	namespace helpers
	{
		inline fs::path create_test_file(const std::string& filename, const std::string& content = "test content")
		{
			fs::path test_file = fs::temp_directory_path() / filename;
			std::ofstream file(test_file);
			file << content;
			file.close();
			return test_file;
		}

		inline fs::path create_test_directory(const std::string& dirname)
		{
			fs::path test_dir = fs::temp_directory_path() / dirname;
			fs::create_directories(test_dir);
			return test_dir;
		}

		inline void cleanup_path(const fs::path& path)
		{
			std::error_code ec;
			if (fs::exists(path, ec))
			{
				if (fs::is_directory(path, ec))
				{
					fs::remove_all(path, ec);
				}
				else
				{
					fs::remove(path, ec);
				}
			}
		}

		class temp_path
		{
		private:
			fs::path path_;

		public:
			~temp_path() { cleanup_path(path_); }

			const fs::path& path() const { return path_; }

			operator const fs::path&() const { return path_; }

			temp_path(const fs::path& p_path) : path_(p_path) {}
		};
	}	 // namespace helpers

	auto run_all_tests() -> void;
}	 // namespace test_utils_filesystem

#endif	  // TEST_UTILS_FILESYSTEM_HPP