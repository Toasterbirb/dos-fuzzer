#pragma once

#include "types.hpp"

#include <string>

namespace fuzz
{
	struct opts
	{
		std::string command_with_orig_bin;
		std::string command_with_patched_bin;
		std::string original_bin_path;
		std::string patched_bin_path;
		u64 section_address;
		u64 section_size;
	};

	opts parse_cli_args(const int argc, char** const argv);
}
