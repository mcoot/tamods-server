#pragma once

#include <string>
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

	// Map
	int searchMapId(const std::map<std::string, int> map, const std::string &str, const std::string &location = "", bool print_on_fail = true);

	// Files
	std::string getConfigDir();
	bool fileExists(const std::string &path, const std::string &mode = "r");
	bool dirExists(const std::string &path);

	extern ATrGameReplicationInfo* tr_gri;
	extern std::mutex tr_gri_mutex;

}