#define _SILENCE_CXX20_CISO646_REMOVED_WARNING
#include "SteamAccount.h"
#include <fstream>
#include <nlohmann/json.hpp>

void CSteamAccountManager::add_account(std::string username, std::string password) {
	add_account(CSteamAccount(username, password));
}

void CSteamAccountManager::add_account(CSteamAccount c_steam_account) {
	steam_accounts.push_back(c_steam_account);
}

void CSteamAccountManager::delete_account(std::string username) {
	for (int i = 0; i < steam_accounts.size(); i++) {
		CSteamAccount c_steam_account = steam_accounts.at(i);
		if (!strcmp(c_steam_account.username.c_str(), username.c_str())) {
			steam_accounts.erase(steam_accounts.begin() + i);
		}
	}
}

void CSteamAccountManager::delete_account(int index) {
	steam_accounts.erase(steam_accounts.begin() + index);
}

void CSteamAccountManager::load_accounts() {
	std::ifstream file;
	file.open("accounts.json");
	if (!file)
		return;
	nlohmann::json json_array = nlohmann::json::parse(file);

	for (auto json_object = json_array.begin(); json_object != json_array.end(); json_object++) {
		std::string username = (*json_object)["username"].get<std::string>();
		std::string password = (*json_object)["password"].get<std::string>();
		add_account(username, password);
	}

	file.close();
}

void CSteamAccountManager::save_accounts() {
	std::ofstream file;
	file.open("accounts.json", std::ios_base::out);
	if (!file)
		file.open("accounts.json", std::fstream::in | std::fstream::out | std::fstream::trunc);
	file.clear();
	nlohmann::json json_array{};

	for (int i = 0; i < steam_accounts.size(); i++) {
		CSteamAccount c_steam_account = steam_accounts.at(i);
		nlohmann::json json_object{};
		json_object["password"] = c_steam_account.password.c_str();
		json_object["username"] = c_steam_account.username.c_str();
		json_array.push_back(json_object);
	}

	file << json_array.dump(2);
	file.close();
}