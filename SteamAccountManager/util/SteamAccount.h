#pragma once
#include <string>
#include <vector>

class CSteamAccount {
public:
	std::string username;
	std::string password;
};

class CSteamAccountManager {
public:
	std::vector<CSteamAccount> steam_accounts{};
	void load_accounts();
	void save_accounts();
	void import_accounts(std::string file_name);
	void export_accounts(std::string file_name);
	void add_account(std::string username, std::string password);
	void add_account(CSteamAccount c_steam_account);
	void delete_account(std::string username);
	void delete_account(int index);
};

inline CSteamAccountManager	c_steam_account_manager;