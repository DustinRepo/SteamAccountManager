#pragma once
#include "SteamAccount.h"
#include <string>

namespace SteamUtil {
	std::string get_steam_exe_path(int process_id);
	int find_steam_pid();
	bool kill_steam_process(int process_id);
	bool start_steam(const char* steam_path, CSteamAccount c_steam_account);
}