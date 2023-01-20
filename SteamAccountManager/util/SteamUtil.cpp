#include "SteamUtil.h"
#include <Windows.h>
#include <TlHelp32.h>//process list
#include <psapi.h>//GetModuleFileNameEx
#include <cstdio>//console
#include <filesystem>//path
#include "SteamAccount.h"

std::string SteamUtil::get_steam_exe_path(int process_id) {
	//Open a handle to the steam process and use GetModuleFileNameEx using NULL as the
	//module name to get the exe name
	HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
	if (!process_handle || process_handle == INVALID_HANDLE_VALUE)
		return std::string();
	char file_path[MAX_PATH];
	if (!GetModuleFileNameEx(process_handle, NULL, file_path, MAX_PATH))
		return std::string();
	CloseHandle(process_handle);
	//For some reason this actually gives a bad string for running the exe
	//so I just cut the file name and place it back myself
	std::filesystem::path steam(file_path);
	steam.remove_filename();
	return steam.string() + "Steam.exe";
}

int SteamUtil::find_steam_pid() {
	//Loop processes, find the process named 'steam.exe' and return it's pid
	HANDLE snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot_handle == INVALID_HANDLE_VALUE)
		return -1;
	PROCESSENTRY32 process_entry{};
	process_entry.dwSize = sizeof(PROCESSENTRY32);
	bool result = Process32First(snapshot_handle, &process_entry);
	while (result) {
		if (!strcmp("steam.exe", process_entry.szExeFile)) {
			return process_entry.th32ProcessID;
		}
		result = Process32Next(snapshot_handle, &process_entry);
	}
	CloseHandle(snapshot_handle);
	return -1;
}

bool SteamUtil::kill_steam_process(int process_id) {
	//Opens a handle to the process and calls TerminateProcess
	HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, process_id);
	if (process_handle == INVALID_HANDLE_VALUE)
		return FALSE;
	bool result = TerminateProcess(process_handle, 1);
	CloseHandle(process_handle);
	return result;
}

bool SteamUtil::start_steam(const char* steam_path, CSteamAccount c_steam_account) {
	//Setup a command line string using -login command with username and password
	//then call CreateProcess, and just close the handles since we aren't using them
	STARTUPINFO startup_info{};
	PROCESS_INFORMATION process_info{};
	startup_info.cb = sizeof(STARTUPINFO);
	char command_line[512]{};
	//command line contains the process name since it will use args[0] if the exe path (steam_path)
	//is null, but it will still expect something there even if you properly pass the path
	sprintf_s(command_line, "steam.exe -login %s %s", c_steam_account.username.c_str(), c_steam_account.password.c_str());
	bool result = CreateProcess(steam_path, command_line, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &process_info);
	if (result) {
		CloseHandle(process_info.hThread);
		CloseHandle(process_info.hProcess);
	}
	return result;
}