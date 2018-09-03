#include "LuaAPI.h"

static std::string dllName = MODNAME;
static std::string dllVersion = MODVERSION;
static std::string taserverProtocolVersion = TASERVER_PROTOCOL_VERSION;
static std::string tamodsProtocolVersion = TAMODS_PROTOCOL_VERSION;

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
					.addVariable("Name", &dllName, false)
					.addVariable("Version", &dllVersion, false)
					.addVariable("TAServerProtocolVersion", &taserverProtocolVersion, false)
					.addVariable("TAModsProtocolVersion", &tamodsProtocolVersion, false)
				.endNamespace()
				// Server modes
				.addVariable("ConnectToTAServer", &g_config.connectToTAServer, true)
				.addVariable("ConnectToClients", &g_config.connectToClients, true)
				.addVariable("AllowUnmoddedClients", &g_config.allowUnmoddedClients, true)
			.endNamespace()

			;
	}
}

