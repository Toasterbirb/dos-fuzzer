#pragma once

#include "types.hpp"

namespace fuzz
{
	struct counter
	{
		constexpr counter(const u64 limit) : limit(limit) {}

		u64 value{0};
		const u64 limit;

		void increment() noexcept { ++value; }
		void reset() noexcept { value = 0; }
		bool has_incremented() const noexcept { return value != 0; };
		bool is_at_limit() const noexcept { return value >= limit; }
	};
}
