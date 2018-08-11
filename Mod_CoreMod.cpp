#include "LuaAPI.h"

static std::string dllName = MODNAME;
static std::string dllVersion = std::to_string(MODVERSION);

namespace LuaAPI {
	void addCoreModAPI(luabridge::Namespace ns) {
		ns
			// Expose the name and version of the mod via Lua
			.beginNamespace("DLL")
				.addVariable("name", &dllName, false)
				.addVariable("version", &dllVersion, false)
			.endNamespace()
			// Logging functions
			.beginNamespace("Logger")
				.addFunction("debug", Logger::printDebug)
				.addFunction("info", Logger::printInfo)
				.addFunction("warn", Logger::printWarn)
				.addFunction("error", Logger::printError)
				.addFunction("fatal", Logger::printFatal)
			.endNamespace()
			;
	}
}

