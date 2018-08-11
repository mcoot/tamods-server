#include "Mods.h"

void ServerSettings::ApplyToGame(ATrGameReplicationInfo* gri) {
	ATrServerSettingsInfo* s = gri->r_ServerConfig;
	// Need to do something different for timelimit...
	gri->TimeLimit = this->TimeLimit;
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

void TrServerSettingsInfo_LoadServerSettings(ATrServerSettingsInfo* that, ATrServerSettingsInfo_eventLoadServerSettings_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	that->eventLoadServerSettings();
	if (!that->Outer->IsA(ATrGameReplicationInfo::StaticClass())) {
		return;
	}
	g_config.serverSettings.ApplyToGame((ATrGameReplicationInfo*)(that->Outer));
	that->ApplyServerSettings();
}

// Nasty nasty getter/setter generation
#define SETTING_GETTERSETTER(type, var) static type get ## var ##() { \
	return g_config.serverSettings.var; \
} \
static void set ## var ##(type n) { \
	Logger::debug("SETTING"); \
	g_config.serverSettings.var = n; \
}

#define SETTING_GETTERSETTER_CAST(type, ctype, var) static type get ## var ##() { \
	return (ctype)g_config.serverSettings.var; \
} \
static void set ## var ##(type n) { \
	g_config.serverSettings.var = (ctype)n; \
}
#define STRINGIFY(x) #x
#define SETTING_LUAPROP(var) addProperty(STRINGIFY(var), &get ## var ## , &set ## var ## )

SETTING_GETTERSETTER(int, TimeLimit);
SETTING_GETTERSETTER(int, WarmupTime);
SETTING_GETTERSETTER(int, OvertimeLimit);
SETTING_GETTERSETTER(int, RespawnTime);

SETTING_GETTERSETTER(int, MaxPlayers);
SETTING_GETTERSETTER_CAST(int, TeamAssignTypes, TeamAssignType);
SETTING_GETTERSETTER_CAST(int, SpawnTypes, SpawnType);
SETTING_GETTERSETTER(bool, AutoBalanceTeams);

SETTING_GETTERSETTER(int, FlagDragLight);
SETTING_GETTERSETTER(int, FlagDragMedium);
SETTING_GETTERSETTER(int, FlagDragHeavy);
SETTING_GETTERSETTER(int, FlagDragDeceleration);

SETTING_GETTERSETTER(bool, FriendlyFire);
SETTING_GETTERSETTER(float, FriendlyFireMultiplier);
SETTING_GETTERSETTER(int, BaseDestructionKickLimit);
SETTING_GETTERSETTER(int, FriendlyFireDamageKickLimit);
SETTING_GETTERSETTER(int, FriendlyFireKillKickLimit);

SETTING_GETTERSETTER(float, EnergyMultiplier);
SETTING_GETTERSETTER(float, AoESizeMultiplier);
SETTING_GETTERSETTER(float, AoEDamageMultiplier);

SETTING_GETTERSETTER(int, LightCountLimit);
SETTING_GETTERSETTER(int, MediumCountLimit);
SETTING_GETTERSETTER(int, HeavyCountLimit);

SETTING_GETTERSETTER(int, ArenaRounds);
SETTING_GETTERSETTER(int, CTFCapLimit);
SETTING_GETTERSETTER(int, TDMKillLimit);
SETTING_GETTERSETTER(int, ArenaLives);
SETTING_GETTERSETTER(int, RabbitScoreLimit);
SETTING_GETTERSETTER(int, CaHScoreLimit);

SETTING_GETTERSETTER(float, VehicleHealthMultiplier);
SETTING_GETTERSETTER(int, GravCycleLimit);
SETTING_GETTERSETTER(int, BeowulfLimit);
SETTING_GETTERSETTER(int, ShrikeLimit);
SETTING_GETTERSETTER(int, GravCycleSpawnTime);
SETTING_GETTERSETTER(int, BeowulfSpawnTime);
SETTING_GETTERSETTER(int, ShrikeSpawnTime);

SETTING_GETTERSETTER(bool, BaseAssets);
SETTING_GETTERSETTER(bool, BaseUpgrades);
SETTING_GETTERSETTER(bool, PoweredDeployables);
SETTING_GETTERSETTER(bool, GeneratorRegen);
SETTING_GETTERSETTER(bool, GeneratorDestroyable);
SETTING_GETTERSETTER(bool, BaseAssetFriendlyFire);
SETTING_GETTERSETTER(bool, DeployableFriendlyFire);

SETTING_GETTERSETTER(bool, SkiingEnabled);
SETTING_GETTERSETTER(bool, CTFBlitzAllFlagsMove);

namespace LuaAPI {
	void addServerSettingsAPI(luabridge::Namespace ns) {
		ns
			.beginNamespace("ServerSettings")
				.SETTING_LUAPROP(TimeLimit)
				.SETTING_LUAPROP(WarmupTime)
				.SETTING_LUAPROP(OvertimeLimit)
				.SETTING_LUAPROP(RespawnTime)

				.SETTING_LUAPROP(MaxPlayers)
				.SETTING_LUAPROP(TeamAssignType)
				.SETTING_LUAPROP(SpawnType)
				.SETTING_LUAPROP(AutoBalanceTeams)

				.SETTING_LUAPROP(FlagDragLight)
				.SETTING_LUAPROP(FlagDragMedium)
				.SETTING_LUAPROP(FlagDragHeavy)
				.SETTING_LUAPROP(FlagDragDeceleration)

				.SETTING_LUAPROP(FriendlyFire)
				.SETTING_LUAPROP(FriendlyFireMultiplier)
				.SETTING_LUAPROP(BaseDestructionKickLimit)
				.SETTING_LUAPROP(FriendlyFireDamageKickLimit)
				.SETTING_LUAPROP(FriendlyFireKillKickLimit)

				.SETTING_LUAPROP(EnergyMultiplier)
				.SETTING_LUAPROP(AoESizeMultiplier)
				.SETTING_LUAPROP(AoEDamageMultiplier)

				.SETTING_LUAPROP(LightCountLimit)
				.SETTING_LUAPROP(MediumCountLimit)
				.SETTING_LUAPROP(HeavyCountLimit)

				.SETTING_LUAPROP(ArenaRounds)
				.SETTING_LUAPROP(CTFCapLimit)
				.SETTING_LUAPROP(TDMKillLimit)
				.SETTING_LUAPROP(ArenaLives)
				.SETTING_LUAPROP(RabbitScoreLimit)
				.SETTING_LUAPROP(CaHScoreLimit)
				.SETTING_LUAPROP(VehicleHealthMultiplier)
				.SETTING_LUAPROP(GravCycleLimit)
				.SETTING_LUAPROP(BeowulfLimit)
				.SETTING_LUAPROP(ShrikeLimit)
				.SETTING_LUAPROP(GravCycleSpawnTime)
				.SETTING_LUAPROP(BeowulfSpawnTime)
				.SETTING_LUAPROP(ShrikeSpawnTime)

				.SETTING_LUAPROP(BaseAssets)
				.SETTING_LUAPROP(BaseUpgrades)
				.SETTING_LUAPROP(PoweredDeployables)
				.SETTING_LUAPROP(GeneratorRegen)
				.SETTING_LUAPROP(GeneratorDestroyable)
				.SETTING_LUAPROP(BaseAssetFriendlyFire)
				.SETTING_LUAPROP(DeployableFriendlyFire)

				.SETTING_LUAPROP(SkiingEnabled)
				.SETTING_LUAPROP(CTFBlitzAllFlagsMove)
			.endNamespace()
			;
	}
}