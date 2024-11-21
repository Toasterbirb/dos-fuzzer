#pragma once

#include "types.hpp"

#include <string>

namespace fuzz
{
	enum mode
	{
		continuous,
		ret,
		time
	};

	struct opts
	{
		std::string command_with_orig_bin;
		std::string command_with_patched_bin;
		std::string original_bin_path;
		std::string patched_bin_path;
		u64 section_address;
		u64 section_size;
		f32 execution_time_variation_multiplier{5.0f};
		u64 max_bytes_to_change{32};

		fuzz::mode mode = mode::continuous;
	};

	opts parse_cli_args(const int argc, char** const argv);
}
