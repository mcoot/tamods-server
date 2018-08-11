#pragma once

#include "Lua.h"
#include "Config.h"

namespace LuaAPI {
	typedef luabridge::Namespace(API)(luabridge::Namespace);

	extern std::vector<API*> api;

	API addCoreModAPI;
}