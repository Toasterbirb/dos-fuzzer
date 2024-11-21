#include "args.hpp"

#include <clipp.h>
#include <format>
#include <iostream>
#include <regex>

namespace fuzz
{
	constexpr char patched_postfix[] = ".patched";

	opts parse_cli_args(const int argc, char** const argv)
	{
		std::string section_address_str;
		std::string section_size_str;
		std::string command;
		opts o;

		bool print_help{false};

		auto cli = (
			(clipp::option("-c", "--cmd").required(true) & clipp::value("command").set(command))
			% "the full command that should be run with the patched file, substitute the path to the file with %c",

			(clipp::option("-f", "--file").required(true) & clipp::value("file_path").set(o.original_bin_path))
			% "path to the file that will be used for fuzzing",

			(clipp::option("-a", "--addr").required(true) & clipp::value("section_address").set(section_address_str))
			% "starting address of the section to fuzz in the binary in hexadecimal format",

			(clipp::option("-s", "--size").required(true) & clipp::value("section_size").set(section_size_str))
			% "the size of the binary section to fuzz in hexadecimal format",

			clipp::one_of(
				clipp::option("-r", "--ret").set(o.mode, mode::ret)
				% "if a non-zero return value is encountered, try to find the minimal amount of changes needed to cause the crash",

				clipp::option("-t", "--time").set(o.mode, mode::time)
				% "if the command execution takes abnormally long, try to find the minimal amount of changes needed to cause the freezing"
			),

			(clipp::option("-v", "--exec-time-variation") & clipp::number("multiplier").set(o.execution_time_variation_multiplier))
			% std::format("the highest normal execution time is multiplied with this value to avoid false positives in case the command just happens to take a bit longer to execute sometimes (default: {})", o.execution_time_variation_multiplier),

			(clipp::option("-b", "--max-bytes-to-change") & clipp::number("count").set(o.max_bytes_to_change))
			% std::format("the maximum about of bytes to change when patching the binary; this value will be truncated to the section size if needed (default: {})", o.max_bytes_to_change),

			clipp::option("-h", "--help").set(print_help) % "print this help page"
		);

		if (!clipp::parse(argc, argv, cli) || print_help)
		{
			std::cout << clipp::make_man_page(cli, "dos-fuzzer");
			exit(1);
		}

		// convert the hex strings into numbers
		o.section_address = std::stoul(section_address_str, 0, 16);
		o.section_size = std::stoul(section_size_str, 0, 16);

		// create the different commands
		const std::regex cmd_regex("%c");
		o.command_with_orig_bin = std::regex_replace(command, cmd_regex, o.original_bin_path);
		o.command_with_patched_bin = std::regex_replace(command, cmd_regex, o.original_bin_path + patched_postfix);

		// create the patched bin path with a postfix
		o.patched_bin_path = o.original_bin_path + patched_postfix;

		return o;
	}
}
