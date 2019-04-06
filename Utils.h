#pragma once

#include <string>
#include <sstream>
#include <mutex>
#include <regex>
#include <map>
#include "SdkHeaders.h"
#include "Logger.h"

namespace Utils {

	// String
	std::string f2std(FString &fstr);
	std::string cleanString(const std::string &str);
	std::string trim(const std::string &str);

	// Map / Classes
	int searchMapId(const std::map<std::string, int> map, const std::string &str, const std::string &location = "", bool print_on_fail = true, int failure_sentinel_value = 0);

	// Files
	std::string getConfigDir();
	bool fileExists(const std::string &path, const std::string &mode = "r");
	bool dirExists(const std::string &path);

	// Perk encoding
	int perks_Encode(int perkA, int perkB);
	int perks_DecodeA(int encoded);
	int perks_DecodeB(int encoded);

	// Server Password Hashing
	std::vector<unsigned char> passwordHash(std::string password);

	enum class ServerGameStatus {
		UNKNOWN = 0,
		PREROUND,
		IN_PROGRESS,
		ENDED
	};

	extern ServerGameStatus serverGameStatus;
	extern ATrGameReplicationInfo* tr_gri;
	extern std::mutex tr_gri_mutex;

}