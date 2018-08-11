#include "LuaAPI.h"

namespace LuaAPI {
	std::vector<API*> api = {
		addCoreModAPI
	};
}

void Lua::init() {
	luabridge::Namespace ns = getGlobalNamespace(_state);
	// Register Lua API components
	for (LuaAPI::API* apiComponent : LuaAPI::api) {
		apiComponent(ns);
	}
}