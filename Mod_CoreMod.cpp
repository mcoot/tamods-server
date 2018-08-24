#include "LuaAPI.h"

static std::string dllName = MODNAME;
static std::string dllVersion = MODVERSION;

static int serverModeStandalone = (int)ServerMode::STANDALONE;
static int serverModeTAServer = (int)ServerMode::TASERVER;
static int serverModeAPI = (int)ServerMode::API;

namespace LuaAPI {
	void addCoreModAPI(luabridge::Namespace ns) {
		ns
			// Logging functions
			.beginNamespace("Logger")
				.addFunction("debug", Logger::printDebug)
				.addFunction("info", Logger::printInfo)
				.addFunction("warn", Logger::printWarn)
				.addFunction("error", Logger::printError)
				.addFunction("fatal", Logger::printFatal)
			.endNamespace()
			.beginNamespace("Core")
				// Expose the name and version of the mod via Lua
				.beginNamespace("DLL")
					.addVariable("name", &dllName, false)
					.addVariable("version", &dllVersion, false)
				.endNamespace()
				// Server modes
				.addVariable("SRV_Standalone", &serverModeStandalone, false)
				.addVariable("SRV_TAServer", &serverModeTAServer, false)
				.addVariable("SRV_API", &serverModeAPI, false)
				.addVariable("ServerMode", (int*)&g_config.serverMode, true)
			.endNamespace()

			;
	}
}

