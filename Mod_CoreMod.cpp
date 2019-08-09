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

static std::string getGConfigDirectory() {
	return g_config.getConfigDirectory();
}

static UTrStatsInterface* getStats() {
	if (!Utils::tr_gri || !Utils::tr_gri->WorldInfo || !Utils::tr_gri->WorldInfo->Game) {
		Logger::debug("[Failed call: no GRI / Game]");
		return NULL;
	}
	UTrStatsInterface* stats = ((ATrGame*)Utils::tr_gri->WorldInfo->Game)->Stats;
	if (!stats) {
		Logger::debug("[Failed call: no stats interface]");
		return NULL;
	}

	return stats;
}

static void callStatsWriteMatchStats() {
	UTrStatsInterface* stats = getStats();
	if (stats) {
		stats->WriteMatchStats();
	}
}

static void callStatsWritePlayerStats() {
	UTrStatsInterface* stats = getStats();
	if (stats) {
		stats->WritePlayerStats(Utils::tr_gri->PRIArray);
	}
}

static void callStatsGameEnd() {
	UTrStatsInterface* stats = getStats();
	if (stats) {
		stats->GameEnd();
	}
}

static void callStatsFlush() {
	UTrStatsInterface* stats = getStats();
	if (stats) {
		stats->Flush();
	}
}

static void callGameOnServerInitialized() {
	Logger::debug("callGameOnServerInitialized's address is %p", &callGameOnServerInitialized);
	if (!Utils::tr_gri || !Utils::tr_gri->WorldInfo || !Utils::tr_gri->WorldInfo->Game) {
		Logger::debug("[Failed call: no GRI / Game]");
		return;
	}
	ATrGame* g = (ATrGame*)Utils::tr_gri->WorldInfo->Game;
	g->OnServerInitialized();
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
				.addFunction("log", &Logger::printLog)
				.addFunction("debug", Logger::printDebug)
				.addFunction("info", Logger::printInfo)
				.addFunction("warn", Logger::printWarn)
				.addFunction("error", Logger::printError)
				.addFunction("fatal", Logger::printFatal)
			.endNamespace()
			.beginNamespace("Core")
				// Get the current config path
				.addProperty<std::string, std::string>("ConfigPath", &getGConfigDirectory)
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
				.addVariable("EventLogging", &g_config.eventLogging, true)
			.endNamespace()
			.beginNamespace("Debug")
				.addFunction("WriteMatchStats", &callStatsWriteMatchStats)
				.addFunction("WritePlayerStats", &callStatsWritePlayerStats)
				.addFunction("GameEnd", &callStatsGameEnd)
				.addFunction("Flush", &callStatsFlush)
				.addFunction("OnServerInitialized", &callGameOnServerInitialized)
			.endNamespace()
			;
	}
}

