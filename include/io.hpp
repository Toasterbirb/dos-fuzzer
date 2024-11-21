#pragma once

#include "types.hpp"

#include <filesystem>
#include <vector>

namespace fuzz
{
	std::vector<u8> read_bytes(const std::filesystem::path path);
	void write_bytes(const std::filesystem::path path, std::vector<u8>& bytes);
	void print_spinner();
	void clear_cli_line();
}
