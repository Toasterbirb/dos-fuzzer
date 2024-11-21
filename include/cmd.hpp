#pragma once

#include "types.hpp"

#include <string_view>

namespace fuzz
{
	struct cmd_res
	{
		// shell or command return value from std::system
		i32 return_value;

		// execution time in milliseconds
		u64 exec_time;
	};

	cmd_res run_cmd(const std::string_view cmd, const u64 execution_time_limit_ms = 1000);
}
