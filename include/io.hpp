#pragma once

#include "cmd.hpp"
#include "types.hpp"

#include <filesystem>
#include <vector>

namespace fuzz
{
	std::vector<u8> read_bytes(const std::filesystem::path& path);
	void write_bytes(const std::filesystem::path path, std::vector<u8>& bytes);
	void print_spinner();
	void clear_cli_line();

	void print_result(const u64 address, const u64 byte_count, const std::vector<u8>& bytes, const cmd_res res);

	__attribute__((noreturn, cold))
	void fatal_error(const std::string& error_msg);
}
