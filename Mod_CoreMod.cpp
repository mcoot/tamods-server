#include "LuaAPI.h"

static std::string dllName = MODNAME;
static std::string dllVersion = MODVERSION;
static std::string taserverProtocolVersion = TASERVER_PROTOCOL_VERSION;
static std::string tamodsProtocolVersion = TAMODS_PROTOCOL_VERSION;

static int logLevelNone = (int)Logger::Level::LOG_NONE;
static int logLevelFatal = (int)Logger::Level::LOG_FATAL;
static int logLevelError = (int)Logger::Level::LOG_ERROR;
static int logLevelWarn = (int)Logger::Level::LOG_WARN;
static int logLevelInfo = (int)Logger::Level::LOG_INFO;
static int logLevelDebug = (int)Logger::Level::LOG_DEBUG;

static void setLoggerLevel(int level) {
	Logger::setLevel((Logger::Level)level);
}

namespace LuaAPI {
	void addCoreModAPI(luabridge::Namespace ns) {
		ns
			// Logging functions
			
			.beginNamespace("Logger")
				.beginNamespace("Levels")
					.addVariable("None", &logLevelNone, false)
					.addVariable("Fatal", &logLevelFatal, false)
					.addVariable("Error", &logLevelError, false)
					.addVariable("Warn", &logLevelWarn, false)
					.addVariable("Info", &logLevelInfo, false)
					.addVariable("Debug", &logLevelDebug, false)
				.endNamespace()
				.addFunction("setLevel", &setLoggerLevel)
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

