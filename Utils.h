#pragma once

#include <string>
#include <regex>
#include <map>
#include "SdkHeaders.h"

namespace Utils {

	// String
	std::string f2std(FString &fstr);

	// Files
	std::string getConfigDir();
	bool fileExists(const std::string &path, const std::string &mode = "r");
	bool dirExists(const std::string &path);

}