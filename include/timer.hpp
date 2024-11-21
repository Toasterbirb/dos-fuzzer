#pragma once

#include <chrono>

#include "types.hpp"

namespace fuzz
{
	class timer
	{
	public:
		void start();
		u64 elapsed_millis() const;

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
	};
}
