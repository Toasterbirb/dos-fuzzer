#include "io.hpp"

#include <fstream>
#include <iostream>

namespace fuzz
{
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
}
