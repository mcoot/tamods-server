#include "Config.h"

// Global config
Config g_config;

Config::Config() {
	reset();
}

Config::~Config() { }

void Config::reset() {
}

void Config::parseFile() { 
	
	std::string directory = Utils::getConfigDir();
	reset();
	lua = Lua();
	if (Utils::fileExists(directory + CONFIGFILE_NAME)) {
		std::string err = lua.doFile(directory + CONFIGFILE_NAME);
		if (err.size()) {
			Logger::error("Lua config error: %s", err.c_str());
			return;
		}
	}
	if (Utils::fileExists(directory + CUSTOMFILE_NAME)) {
		std::string err = lua.doFile(directory + CUSTOMFILE_NAME);
		if (err.size()) {
			Logger::error("Custom Lua config error: %s", err.c_str());
			return;
		}
	}

	setConfigVariables();
}

#define SET_VARIABLE(type, var) (lua.setVar<type>(var, #var))
#define SET_FUNCTION(var) do {\
	var = new LuaRef(getGlobal(lua.getState(), #var));\
	if (var && (var->isNil() || !var->isFunction())) {\
		delete var;\
		var = NULL;\
	}\
} while (0)

// Set the TAMods config based on the current state of the Lua object
void Config::setConfigVariables() {
}

void ServerSettings::ApplyToGame(ATrServerSettingsInfo* s) {
	// Need to do something different for timelimit...
	s->TimeLimit = this->TimeLimit;
	s->WarmupTime = this->WarmupTime;
	s->OvertimeLimit = this->OvertimeLimit;
	s->RespawnTime = this->RespawnTime;

	s->MaxPlayers = this->MaxPlayers;
	s->TeamAssignType = (int)this->TeamAssignType;
	s->SpawnType = (int)this->SpawnType;
	s->bAutoBalanceInGame = this->AutoBalanceTeams;

	s->MaxSpeedWithFlagLight = this->FlagDragLight;
	s->MaxSpeedWithFlagMedium = this->FlagDragMedium;
	s->MaxSpeedWithFlagHeavy = this->FlagDragHeavy;
	s->DecelerationRateWithFlag = this->FlagDragDeceleration;

	s->bFriendlyFire = this->FriendlyFire;
	s->fFriendlyFireDamageMultiplier = this->FriendlyFireMultiplier;
	s->BaseDestructionLimit = this->BaseDestructionKickLimit;
	s->FFDamageLimit = this->FriendlyFireDamageKickLimit;
	s->FFKillLimit = this->FriendlyFireKillKickLimit;

	//s->fEnergyMultiplier = this->EnergyMultiplier;
	//s->fAoESizeMultiplier = this->AoESizeMultiplier;
	//s->fAoEDamageMultiplier = this->AoEDamageMultiplier;

	//// Verify how this indexing works now...
	s->ClassCounts[0] = this->LightCountLimit;
	s->ClassCounts[1] = this->MediumCountLimit;
	s->ClassCounts[2] = this->HeavyCountLimit;

	s->ArenaRounds = this->ArenaRounds;
	s->GameScores[TGT_CTF] = this->CTFCapLimit;
	s->GameScores[TGT_TDM] = this->TDMKillLimit;
	s->GameScores[TGT_ARN] = this->ArenaLives;
	s->GameScores[TGT_RAB] = this->RabbitScoreLimit;
	s->GameScores[TGT_CAH] = this->CaHScoreLimit;

	s->fVehicleHealthMultiplier = this->VehicleHealthMultiplier;
	s->VehicleLimits[VEHICLE_GravCycle] = this->GravCycleLimit;
	s->VehicleLimits[VEHICLE_Beowulf] = this->BeowulfLimit;
	s->VehicleLimits[VEHICLE_Shrike] = this->ShrikeLimit;
	s->VehicleTimes[VEHICLE_GravCycle] = this->GravCycleSpawnTime;
	s->VehicleTimes[VEHICLE_Beowulf] = this->BeowulfSpawnTime;
	s->VehicleTimes[VEHICLE_Shrike] = this->ShrikeSpawnTime;

	s->bPreplacedObjectives = this->BaseAssets;
	s->bObjectiveUpgrades = this->BaseUpgrades;
	s->bPoweredDeployables = this->PoweredDeployables;
	s->bGeneratorAutoRegen = this->GeneratorRegen;
	s->bGenDestroyable = this->GeneratorDestroyable;
	s->bFriendlyFireBaseAssets = this->BaseAssetFriendlyFire;
	s->bFriendlyFireDeployables = this->DeployableFriendlyFire;

	s->bSkiEnabled = this->SkiingEnabled;
	s->bCTFBlitzAllFlagsMove = this->CTFBlitzAllFlagsMove;
}