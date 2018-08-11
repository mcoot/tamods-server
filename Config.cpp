#include "Config.h"

// Global config
Config g_config;

Config::Config() {
	reset();
}

Config::~Config() { }

void Config::reset() {
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

