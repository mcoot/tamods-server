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
	hardcodedLoadouts = HardCodedLoadouts();
}

void Config::parseFile() { 
	
	std::string directory = Utils::getConfigDir();
	reset();
	lua = Lua();
	if (Utils::fileExists(directory + CONFIGFILE_NAME)) {
		std::string err = lua.doFile(directory + CONFIGFILE_NAME);
		if (err.size()) {
			Logger::error("Lua config error: %s", err.c_str());
			return;
		}
	}
	if (Utils::fileExists(directory + CUSTOMFILE_NAME)) {
		std::string err = lua.doFile(directory + CUSTOMFILE_NAME);
		if (err.size()) {
			Logger::error("Custom Lua config error: %s", err.c_str());
			return;
		}
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

void Config::load() {
	parseFile();

	// If there is no map rotation, add the default rotation
	if (serverSettings.mapRotation.empty()) {
		addDefaultMapRotation();
	}
}