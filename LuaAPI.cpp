#include "LuaAPI.h"

namespace LuaAPI {
	std::vector<API*> api = {
		addCoreModAPI,
		addServerSettingsAPI,
		addLoadoutsLuaAPI,
		addGameBalanceAPI,
		addAdministrationAPI,
	};
}

void Lua::init() {
	// Register Lua API components
	for (LuaAPI::API* apiComponent : LuaAPI::api) {
		apiComponent(getGlobalNamespace(_state));
	}
}