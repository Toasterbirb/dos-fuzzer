#include "io.hpp"

#include <array>
#include <fstream>
#include <iostream>

namespace fuzz
{
	// different charcters to use for a loading spinner
	constexpr std::array spinner_chars = { '-', '\\', '|', '/' };

	// current index of the spinner char to print
	// shouldn't matter if this overflows
	u8 spinner_char_index{0};

	std::vector<u8> read_bytes(const std::filesystem::path path)
	{
		std::ifstream file(path);
		file.seekg(0, std::ios::end);

		std::vector<u8> bytes(file.tellg());

		file.seekg(0, std::ios::beg);
		file.read((char*)&bytes[0], bytes.size());
		return bytes;
	}

	void write_bytes(const std::filesystem::path path, std::vector<u8>& bytes)
	{
		if (!std::filesystem::is_regular_file(path))
		{
			std::cout << path << " is a directory and not a file!\n";
			abort();
		}

		// if the file exists already, get rid of it
		std::filesystem::remove(path);

		std::ofstream file(path);
		file.write((char*)&bytes[0], bytes.size());
	}

	void print_spinner()
	{
		std::cout << "\033[2K\r[" << spinner_chars.at(++spinner_char_index % spinner_chars.size()) << ']' << std::flush;
	}

	void clear_cli_line()
	{
		std::cout << "\033[2K\r";
	}

	void print_result(const u64 address, const u64 byte_count, const std::vector<u8>& bytes,
			const cmd_res res, const u64 expected_execution_time)
	{
		std::cerr << std::hex << "0x" << address << " | ";
		for (u64 i = address; i < address + byte_count; ++i)
			std::fprintf(stderr, "%02x ", bytes.at(i));

		std::cerr << "| ";

		if (res.exec_time > expected_execution_time)
			std::cerr << "time (" << std::dec << res.exec_time << "ms) ";

		if (res.return_value != 0)
			std::cerr << "ret ";

		std::cerr << std::endl;
	}
}
