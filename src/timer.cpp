#include "timer.hpp"

namespace fuzz
{
	void timer::start()
	{
		start_time = std::chrono::high_resolution_clock::now();
	}

	u64 timer::elapsed_millis() const
	{
		const auto current_time = std::chrono::high_resolution_clock::now();
		const auto duration = std::chrono::duration(current_time - start_time);
		return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	}
}
