#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <random>
#include <unordered_map>
#include <unordered_set>

#include "args.hpp"
#include "cmd.hpp"
#include "counter.hpp"
#include "io.hpp"
#include "timer.hpp"

int main(int argc, char** argv)
{
	// parsing CLI args is done in a separate compilation unit because
	// clipp and std::regex have horrendous compile times
	const fuzz::opts opts = fuzz::parse_cli_args(argc, argv);


	const u8 bytes_to_change = opts.section_size < opts.max_bytes_to_change ? opts.section_size : opts.max_bytes_to_change;

	// read in the original binary
	std::vector<u8> orig_bytes = fuzz::read_bytes(opts.original_bin_path);

	if (orig_bytes.size() < opts.section_address + opts.section_size)
	{
		std::cout << "part of the defined section goes outside the bounds of the binary file\n";
		return 1;
	}

	// execute the command a few times to figure out the expected runtime

	fuzz::timer t;
	u64 longest_execution_time{0};

	std::cout << "testing normal execution time with " << std::dec << (u32)opts.test_run_count << " runs\n";
	for (u8 i = 0; i < opts.test_run_count; ++i)
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
	const u64 expected_execution_time = longest_execution_time * opts.execution_time_variation_multiplier;
	std::cout << "longest normal execution time: " << longest_execution_time << "ms\n";
	std::cout << "execution time limit: " << expected_execution_time << "ms\n";


	// seed the random number generator
	const u64 seed = opts.seed == 0 ? std::chrono::high_resolution_clock::now().time_since_epoch().count() : opts.seed;
	std::cout << "seed: " << seed << '\n';
	std::srand(seed);

	// if continuous mode is used, loop infinitely and try making different
	// changes to the binary and see what happens
	//
	// if ret or time modes are used, stop at the first anomaly
	std::cout << "fuzzing the binary section at 0x" << std::hex << opts.section_address << std::endl;

	// if using something other than the continuous mode, store working bytes
	// based on their locations into vectors
	// they will then be randomly tried when looking for better areas
	//
	// shouldn't matter if the same byte appears multiple times at the same location
	// afterall, that just means that that byte should be good at causing trouble
	// and should be tried more often
	std::unordered_map<u64, std::vector<u8>> byte_cache;

	// these variables are stored outside the scope of the continuous loop
	// since they are needed for the refining loop later on if the continuous mode is not used
	u64 start_address{0};
	u64 end_address{0};

	while (true)
	{
		// print a spinner
		// this should help with seeing if the program we are testing has frozen
		fuzz::print_spinner();

		const u64 byte_count = (std::rand() % (bytes_to_change - 1)) + 1;
		const u64 start_byte = std::rand() % (opts.section_size - byte_count);

		std::vector<u8> patched_bytes = orig_bytes;
		start_address = opts.section_address + start_byte;
		end_address = opts.section_address + start_byte + byte_count;

		for (u64 i = start_address; i < end_address; ++i)
			patched_bytes.at(i) = std::rand() % 255;

		// write the patched binary to disk
		fuzz::write_bytes(opts.patched_bin_path, patched_bytes);

		// attempt to execute the command with the patched binary
		fuzz::cmd_res res = fuzz::run_cmd(opts.command_with_patched_bin, expected_execution_time);

		const bool time_result = res.exec_time > expected_execution_time;
		const bool ret_result = res.return_value != 0;
		if (time_result || ret_result)
		{
			// clear the spinner from the current line
			fuzz::clear_cli_line();

			fuzz::print_result(start_address, byte_count, patched_bytes, res);

			// if any other mode than continuous is used, stop right here
			if (opts.mode != fuzz::mode::continuous && ((time_result && opts.mode == fuzz::mode::time) || (ret_result && opts.mode == fuzz::mode::ret)))
			{
				// cache the troublesome bytes
				for (u64 i = start_address; i < end_address; ++i)
					byte_cache[i].push_back(patched_bytes.at(i));

				break;
			}
		}
	}

	std::cout << (opts.mode == fuzz::mode::time ? "long execution time" : "non-zero exit code") << " was encountered\n"
		<< "starting to look for the minimal amount of changes needed for reproduction...\n";

	// cache for holding byte combinations that have already been tried before
	// to avoid doing duplicate work
	//
	// this is needed due to the byte_cache having a high risk of duplicate byte
	// combinations when the search area becomes smaller
	//
	// the runtime of the program is very likely to be higher than hashing a string
	// of a few bytes
	std::unordered_map<u64, std::unordered_set<std::string>> patched_bytes_cache;

	fuzz::counter patch_bytes_skip(opts.max_bytes_to_change);

	// how many times we have had to do a loop skip because a singular
	// byte ran out of combinations
	fuzz::counter single_byte_skip(opts.max_bytes_to_change);

	// loop until we are down to a singular byte
	u64 min_patch_size = end_address - start_address;

	// calculate a multiplier for the byte area size in the patching loop
	// this should make it so that when the area gets smaller, the cached
	// bytes are used less and less to favor entirely random bytes
	//
	// this should prevent exhausint byte combinations very early on
	// due to only using cached bytes
	const f32 byte_cache_rng_threshold = 1.0f / std::clamp(opts.max_bytes_to_change * 2.0f, 4.0f, 64.0f);


	while (min_patch_size > 2)
	{
		fuzz::print_spinner();
		std::cout << " search area: " << std::dec << end_address - start_address << " bytes" << std::flush;

		std::vector<u8> patched_bytes = orig_bytes;

		// spam random address ranges until we get something that has less
		// bytes than the current minimum
		//
		// kind of a naive approach, but it shall do for now

		u64 min_start_address;
		u64 min_end_address;

		do
		{
			// if the single single_byte_skip_counter has reached its limit, stop
			// generating areas that are only a singular byte in size
			const u8 min_area_size = single_byte_skip.is_at_limit()
				? 2
				: 1;

			min_start_address = start_address + (rand() % (end_address - start_address - min_area_size));
			min_end_address = end_address - (rand() % (end_address - min_start_address));

		} while (min_end_address - min_start_address >= min_patch_size && min_end_address > min_start_address);

		assert(min_start_address < patched_bytes.size());
		assert(min_end_address < patched_bytes.size());
		assert(min_end_address - min_start_address > 0);

		// loop until a new combination of patches bytes that isn't in the
		// patched_bytes_cache gets generated
		//
		// if a new combination cannot be found within a certain amount of attempts,
		// loop around to attempt to get an address range that works better
		//
		// if this skip has to be done multiple times in a row, we'll stop
		// the search entirely and call it quits because we can't come up
		// with new combinations to try with in a reasonable amount of time
		//
		// at that point the amount of bytes left should be pretty small anyway
		std::string byte_str;

		fuzz::counter patch_bytes_loop(100'000);

		do
		{
			patch_bytes_loop.increment();

			// start with a new byte string on each iteration
			byte_str.clear();

			// patch the bytes
			for (u64 i = min_start_address; i < min_end_address; ++i)
			{
				const f32 rng = rand() / static_cast<f32>(RAND_MAX);

				// use the cached bytes randomly
				//
				// the less bytes there are left, the less the cache should be used
				// since its faster to iterate through different random combinations
				//
				// also if the cache is used heavily with very few bytes left,
				// there might be a lot of wasted rounds due to the same combination
				// being tested multiple times
				//
				// if the patch_bytes_skip_counter has been touched, stop using the cache
				if (patch_bytes_skip.has_incremented() && rng > (byte_cache_rng_threshold * (min_end_address - min_start_address)))
				{
					patched_bytes.at(i) = byte_cache.at(i).at(rand() % byte_cache.at(i).size());
					continue;
				}

				// try 00 and FF slightly more often if we haven't had to skip a loop yet
				if (patch_bytes_skip.has_incremented() && rand() % 128 == 0)
				{
					patched_bytes.at(i) = rand() % 2 == 0 ? 0x00 : 0xFF;
					continue;
				}

				patched_bytes.at(i) = rand() % 256;
			}

			// copy the bytes into a string and use the unordered_set to hash it and cache it
			// (assuming that it isn't already in the cache)

			std::copy(patched_bytes.begin() + min_start_address, patched_bytes.begin() + min_end_address, std::back_inserter(byte_str));
		} while (patched_bytes_cache[min_start_address].contains(byte_str) && !patch_bytes_loop.is_at_limit());

		// we have had to loop around too many times, stop searching
		if (patch_bytes_skip.is_at_limit())
		{
			fuzz::clear_cli_line();
			std::cout << "cannot come up with new byte combinations anymore in a reasonable amount of time\n"
				<< "giving up (╯°□°）╯︵ ┻━┻\n";
			break;
		}

		if (patch_bytes_loop.is_at_limit())
		{
			// increment the skip counter only if the area we run out of combinations with
			// was larger than a singular byte
			//
			// one byte only has 256 different combinations and exhasuting that list
			// takes no effort at all; 2+ bytes should be a different story
			if (min_end_address - min_start_address > 1)
			{
				if (!patch_bytes_skip.has_incremented())
				{
					fuzz::clear_cli_line();
					std::cout << "running out of byte combinations to try\ndisabling byte cache...\n";
				}

				patch_bytes_skip.increment();
			}
			else
			{
				if (!single_byte_skip.has_incremented())
				{
					fuzz::clear_cli_line();
					std::cout << "giving up on finding a 1 byte solution\n";
				}

				single_byte_skip.increment();
			}
			continue;
		}

		// cache the byte combination
		patched_bytes_cache[min_start_address].insert(byte_str);

		fuzz::write_bytes(opts.patched_bin_path, patched_bytes);
		fuzz::cmd_res res = fuzz::run_cmd(opts.command_with_patched_bin, expected_execution_time);

		const bool time_result = opts.mode == fuzz::mode::time && res.exec_time > expected_execution_time;
		const bool ret_result = opts.mode == fuzz::mode::ret && res.return_value != 0;
		if (time_result || ret_result)
		{
			fuzz::clear_cli_line();

			fuzz::print_result(min_start_address, min_end_address - min_start_address, patched_bytes, res);
			min_patch_size = min_end_address - min_start_address;

			// nudge the search area to the hopefully correct direction by limiting it to
			// the bytes around the latest result
			//
			// basically if the starting point of the found position is equal to the previously
			// found point, we increase the search radius by one byte to that direction
			//
			// same goes for the end address aswell
			//
			// however if there are only 1-2 bytes left, don't make the area
			// smaller so that it is easier to check for a 1 byte solution
			if (min_patch_size > 2)
			{
				start_address = start_address == min_start_address ? min_start_address - 1 : min_start_address;
				end_address = end_address == min_end_address ? min_end_address + 1 : min_end_address;
			}
			else
			{
				start_address = min_start_address;
				end_address = min_end_address;
			}

			// cache the bytes at the new area
			for (u64 i = start_address; i < end_address; ++i)
				byte_cache[i].push_back(patched_bytes.at(i));
		}
	}

	// if we were left with two bytes at the end, try all possible combinations
	// (that haven't been tried before) to see if there would be a 1 byte solution
	if (min_patch_size == 2)
	{
		std::cout << "trying all possible combinations to find a 1 byte solution\n" << std::flush;
		std::string byte_str;
		byte_str.resize(1);

		const auto try_byte = [&opts, &orig_bytes, expected_execution_time](const u64 addr, const u8 byte) -> bool
		{
			std::vector<u8> patched_bytes = orig_bytes;
			patched_bytes.at(addr) = byte;
			fuzz::write_bytes(opts.patched_bin_path, patched_bytes);
			fuzz::cmd_res res = fuzz::run_cmd(opts.command_with_patched_bin, expected_execution_time);

			const bool time_result = res.exec_time > expected_execution_time;
			const bool ret_result = res.return_value != 0;

			if (time_result || ret_result)
				fuzz::print_result(addr, 1, patched_bytes, res);

			return (time_result && opts.mode == fuzz::mode::time) || (ret_result && opts.mode == fuzz::mode::ret);
		};

		// brute force both bytes until a solution is found
		constexpr u8 bytes_to_bruteforce = 2;
		for (u8 byte = 0; byte < bytes_to_bruteforce; ++byte)
		{
			std::cout << "byte[" << std::dec << (i32)byte << "] at 0x" << std::hex << start_address + byte << '\n';

			bool solution_found{false};
			for (u16 i = 0; i < 256; ++i)
			{
				byte_str[0] = i;

				if (patched_bytes_cache[start_address + byte].contains(byte_str))
					continue;

				solution_found = try_byte(start_address + byte, i);

				if (solution_found)
					break;
			}

			if (solution_found)
				break;
		}
	}

	return 0;
}
