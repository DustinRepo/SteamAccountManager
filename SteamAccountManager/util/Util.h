#pragma once
#include <string>

#define XOR_KEY "SteamAccountManager"

namespace Util {
	std::string xor_string(std::string input, std::string key);
	std::string open_file_dialog();
}