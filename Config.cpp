#include "Config.h"

// Global config
Config g_config;

Config::Config() {
	reset();
}

Config::~Config() { }

void Config::reset() {
	serverSettings = ServerSettings();
	connectToTAServer = true;
	connectToClients = true;
	allowUnmoddedClients = true;
	eventLogging = false;
	hardcodedLoadouts = HardCodedLoadouts();
}

GameBalance::ReplicatedSettings Config::getReplicatedSettings() {
	GameBalance::ReplicatedSettings repSet;

	repSet["EnableInventoryCallIn"] = serverSettings.EnableInventoryCallIn;
	repSet["InventoryCallInCooldownTime"] = serverSettings.InventoryCallInCooldownTime;
	repSet["InventoryCallInBuildUpTime"] = serverSettings.InventoryCallInBuildUpTime;
	repSet["RageThrustPackDependsOnCapperSpeed"] = serverSettings.RageThrustPackDependsOnCapperSpeed;

	repSet["UseGOTYBXTCharging"] = serverSettings.UseGOTYBXTCharging;

	return repSet;
}

void Config::parseFile(std::string filePath) { 
	reset();
	
	size_t last_sep = filePath.find_last_of("\\/");
	if (last_sep == std::string::npos) {
		Logger::error("Invalid filename %s, could not split working directory", filePath.c_str());
		return;
	}
	std::string workingDir(filePath);
	workingDir.erase(last_sep + 1, workingDir.length());
	lua = Lua(workingDir);

	if (Utils::fileExists(filePath)) {
		Logger::info("Loading configuration from %s", filePath.c_str());
		std::string err = lua.doFile(filePath);
		if (err.size()) {
			Logger::error("Lua config error: %s", err.c_str());
			return;
		}
	}
	else {
		Logger::error("Unable to locate config file %s", filePath.c_str());
	}

	setConfigVariables();
}

#define SET_VARIABLE(type, var) (lua.setVar<type>(var, #var))
#define SET_FUNCTION(var) do {\
	var = new LuaRef(getGlobal(lua.getState(), #var));\
	if (var && (var->isNil() || !var->isFunction())) {\
		delete var;\
		var = NULL;\
	}\
} while (0)

// Set the TAMods config based on the current state of the Lua object
void Config::setConfigVariables() {
}

void Config::addDefaultMapRotation() {
	serverSettings.mapRotation.push_back("TrCTF-Katabatic");
	serverSettings.mapRotation.push_back("TrCTF-ArxNovena");
	serverSettings.mapRotation.push_back("TrCTF-DangerousCrossing");
	serverSettings.mapRotation.push_back("TrCTF-Crossfire");
	serverSettings.mapRotation.push_back("TrCTF-Drydock");
	serverSettings.mapRotation.push_back("TrCTF-Terminus");
	serverSettings.mapRotation.push_back("TrCTF-Sunstar");
}

#define CLI_FLAG_CONFIGPATH L"-tamodsconfig"

static std::string getConfigFilePath() {
	int numArgs;
	wchar_t** args = CommandLineToArgvW(GetCommandLineW(), &numArgs);

	if (!args) {
		return Utils::getConfigDir() + CONFIGFILE_NAME;
	}

	// Only check up to the second last argument
	// Since we need both the flag and the path
	for (int i = 0; i < numArgs - 1; ++i) {
		if (wcscmp(args[i], CLI_FLAG_CONFIGPATH) == 0) {
			// User has set the config path
			// This won't work if there are non-ascii characters in the path but yolo
			std::wstring cfgPathW(args[i + 1]);
			std::string cfgPath(cfgPathW.begin(), cfgPathW.end());
			return cfgPath;
		}
	}

	return Utils::getConfigDir() + CONFIGFILE_NAME;
}

std::string Config::getConfigDirectory() {
	std::string cfgPath = getConfigFilePath();

	size_t last_sep = cfgPath.find_last_of("\\/");
	std::string workingDir(cfgPath);
	workingDir.erase(last_sep + 1, workingDir.length());
	
	return workingDir;
}

void Config::load() {
	parseFile(getConfigFilePath());

	// If there is no map rotation, add the default rotation
	if (serverSettings.mapRotation.empty()) {
		addDefaultMapRotation();
	}
}