#pragma once

#include "Lua.h"
#include "Config.h"

namespace LuaAPI {
	typedef void(API)(luabridge::Namespace ns);

	extern std::vector<API*> api;

	API addCoreModAPI;
	API addServerSettingsAPI;
	API addLoadoutsLuaAPI;
}