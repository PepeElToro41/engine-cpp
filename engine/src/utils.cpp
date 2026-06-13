#include "utils.hpp"

#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>

std::string read_text_file(const std::string &file_path)
{
	std::ifstream infile(file_path);
	if (infile.is_open()) {
		std::stringstream buffer;
		buffer << infile.rdbuf();
		const std::string output = buffer.str();
		infile.close();
		return output;
	}
	return std::string();
}