#include "cmd.hpp"
#include "timer.hpp"

#include <cstdlib>
#include <future>
#include <iostream>

namespace fuzz
{
	using namespace std::chrono_literals;

	constexpr std::chrono::milliseconds hanging_command_poll_time = 250ms;

	cmd_res run_cmd(const std::string_view cmd, const u64 execution_time_limit_ms)
	{
		timer t;
		cmd_res res;

		t.start();

		std::future<i32> cmd_future = std::async(std::launch::async, std::system, cmd.data());

		if (cmd_future.wait_for(std::chrono::milliseconds(execution_time_limit_ms)) != std::future_status::ready)
		{
			while (cmd_future.wait_for(hanging_command_poll_time) != std::future_status::ready)
			{
				std::cout << "\033[2K\rcommand has been hanging for " << std::dec << t.elapsed_millis() / 1000.0f << "s" << std::flush;
			}
		}

		res.return_value = cmd_future.get();
		res.exec_time = t.elapsed_millis();

		return res;
	}
}
