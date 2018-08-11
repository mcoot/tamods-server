#pragma once

#include <fstream>
#include <string>
#include <regex>
#include <queue>
#include <map>

#include "buildconfig.h"

#include "SdkHeaders.h"
#include "Geom.h"
#include "Lua.h"
#include "Logger.h"
#include "Utils.h"
#include "Hooks.h"
#include "Data.h"

class Config {
public:
	Config();
	~Config();

	void reset();
	void parseFile();
	void setConfigVariables();

public:
	Lua lua;
};

extern Config g_config;