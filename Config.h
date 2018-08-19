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
#include "LoadoutTypes.h"


struct ServerSettings {
	int TimeLimit = 25;
	int WarmupTime = 20;
	int OvertimeLimit = 10;
	int RespawnTime = 5;

	int MaxPlayers = 32;
	TeamAssignTypes TeamAssignType = TAT_BALANCED;
	SpawnTypes SpawnType = EST_NORMAL;
	bool AutoBalanceTeams = true;

	int FlagDragLight = 0;
	int FlagDragMedium = 0;
	int FlagDragHeavy = 0;
	int FlagDragDeceleration = 0;

	bool FriendlyFire = false;
	float FriendlyFireMultiplier = 1.0f;
	int BaseDestructionKickLimit = 1000;
	int FriendlyFireDamageKickLimit = 1000;
	int FriendlyFireKillKickLimit = 5;

	float EnergyMultiplier = 1000.0f;
	float AoESizeMultiplier = 1000.0f;
	float AoEDamageMultiplier = 1000.0f;

	int LightCountLimit = 32;
	int MediumCountLimit = 32;
	int HeavyCountLimit = 32;

	int ArenaRounds = 3;
	int CTFCapLimit = 5;
	int TDMKillLimit = 100;
	int ArenaLives = 25;
	int RabbitScoreLimit = 30;
	int CaHScoreLimit = 50;

	float VehicleHealthMultiplier = 1.0f;
	int GravCycleLimit = 4;
	int BeowulfLimit = 2;
	int ShrikeLimit = 2;
	int GravCycleSpawnTime = 30;
	int BeowulfSpawnTime = 120;
	int ShrikeSpawnTime = 60;

	bool BaseAssets = true;
	bool BaseUpgrades = true;
	bool PoweredDeployables = true;
	bool GeneratorRegen = false;
	bool GeneratorDestroyable = true;
	bool BaseAssetFriendlyFire = false;
	bool DeployableFriendlyFire = false;

	bool SkiingEnabled = true;
	bool CTFBlitzAllFlagsMove = false;

	void ApplyToGame(ATrGameReplicationInfo* gri);
};

enum class ServerMode {
	STANDALONE,
	TASERVER,
	API
};

class Config {
public:
	Config();
	~Config();

	void reset();
	void parseFile();
	void setConfigVariables();

public:
	Lua lua;

	ServerSettings serverSettings;

	ServerMode serverMode;
	HardCodedLoadouts hardcodedLoadouts;
};

extern Config g_config;