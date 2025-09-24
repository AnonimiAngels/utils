#include "test_interface.hpp"

auto test_api::initialize() -> void
{
	spdlog::set_level(spdlog::level::warn);
}