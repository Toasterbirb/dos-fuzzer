#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>

#include "args.hpp"
#include "cmd.hpp"
#include "io.hpp"
#include "timer.hpp"

int main(int argc, char** argv)
{
	// parsing CLI args is done in a separate compilation unit because
	// clipp and std::regex have horrendous compile times
	const fuzz::opts opts = fuzz::parse_cli_args(argc, argv);


	constexpr u8 max_bytes_to_change = 32;
	const u8 bytes_to_change = opts.section_size < max_bytes_to_change ? opts.section_size : max_bytes_to_change;

	// read in the original binary
	std::vector<u8> orig_bytes = fuzz::read_bytes(opts.original_bin_path);

	if (orig_bytes.size() < opts.section_address + opts.section_size)
	{
		std::cout << "part of the defined section goes outside the bounds of the binary file\n";
		return 1;
	}

	// execute the command a few times to figure out the expected runtime

	fuzz::timer t;
	constexpr u8 test_run_count = 10;
	u64 longest_execution_time{0};

	std::cout << "testing normal execution time with " << std::dec << (u32)test_run_count << " runs\n";
	for (u8 i = 0; i < test_run_count; ++i)
	{
		fuzz::cmd_res res = fuzz::run_cmd(opts.command_with_orig_bin);

		if (res.exec_time > longest_execution_time)
			longest_execution_time = res.exec_time;

		if (res.return_value) [[unlikely]]
		{
			std::cout << "error!\n"
				<< "running the command with the original binary has a non-zero return value\n";
			return 1;
		}
	}

	// allow for some extra time for the execution in case
	// the program just happens to take a little bit longer sometimes
	constexpr f32 execution_time_variation_multiplier = 3;
	const u64 expected_execution_time = longest_execution_time * execution_time_variation_multiplier;
	std::cout << "longest normal execution time: " << longest_execution_time << "ms\n";
	std::cout << "execution time limit: " << expected_execution_time << "ms\n";


	// seed the random number generator
	std::srand(std::chrono::high_resolution_clock::now().time_since_epoch().count());

	// different charcters to use for a loading spinner
	constexpr std::array spinner_chars = { '-', '\\', '|', '/' };

	u8 spinner_char_index{0}; // shouldn't matter if this overflows

	// an infinite loop where we try to make random changes to the binary
	// and see if things break or not
	std::cout << "fuzzing the binary section at 0x" << std::hex << opts.section_address << std::endl;
	while (true)
	{
		// print a spinner
		// this should help with seeing if the program we are testing has frozen
		std::cout << "\033[2K\r[" << spinner_chars.at(++spinner_char_index % spinner_chars.size()) << ']' << std::flush;

		const u64 byte_count = (std::rand() % (bytes_to_change - 1)) + 1;
		const u64 start_byte = std::rand() % (opts.section_size - byte_count);

		std::vector<u8> patched_bytes = orig_bytes;
		const u64 start_address = opts.section_address + start_byte;
		const u64 end_address = opts.section_address + start_byte + byte_count;

		for (u64 i = start_address; i < end_address; ++i)
			patched_bytes.at(i) = std::rand() % 255;

		// write the patched binary to disk
		fuzz::write_bytes(opts.patched_bin_path, patched_bytes);

		// attempt to execute the command with the patched binary
		fuzz::cmd_res res = fuzz::run_cmd(opts.command_with_patched_bin, expected_execution_time);

		if (res.exec_time > expected_execution_time || res.return_value != 0)
		{
			// clear the spinner from the current line
			std::cout << "\033[2K\r";

			std::cerr << std::hex << "0x" << start_address << " | ";
			for (u64 i = start_address; i < end_address; ++i)
				std::fprintf(stderr, "%02x ", patched_bytes.at(i));

			std::cerr << "| ";

			if (res.exec_time > expected_execution_time)
				std::cerr << "time (" << std::dec << res.exec_time << "ms) ";

			if (res.return_value != 0)
				std::cerr << "ret ";

			std::cerr << std::endl;
		}
	}

	return 0;
}
