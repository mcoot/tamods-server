#include "Mods.h"

void ServerSettings::ApplyAsDefaults() {
	ATrServerSettingsInfo* defaultServerSettings = ((ATrServerSettingsInfo*)ATrServerSettingsInfo::StaticClass()->Default);
	this->ApplyToGame(defaultServerSettings);
}

void ServerSettings::ApplyToGame(ATrServerSettingsInfo* s) {
	ATrPlayerController* defTrPC = (ATrPlayerController*)(ATrPlayerController::StaticClass()->Default);

	s->TimeLimit = this->TimeLimit;
	s->WarmupTime = this->WarmupTime;
	s->OvertimeLimit = this->OvertimeLimit;
	s->RespawnTime = this->RespawnTime;
	defTrPC->fSniperRespawnDelay = (float)this->SniperRespawnDelay;

	s->MaxPlayers = this->MaxPlayers;
	s->TeamAssignType = (int)this->TeamAssignType;
	s->SpawnType = this->NakedSpawn ? EST_NAKEDPTH : EST_NORMAL;
	s->bAutoBalanceInGame = this->AutoBalanceTeams;

	s->MaxSpeedWithFlagLight = this->FlagDragLight;
	s->MaxSpeedWithFlagMedium = this->FlagDragMedium;
	s->MaxSpeedWithFlagHeavy = this->FlagDragHeavy;
	s->DecelerationRateWithFlag = this->FlagDragDeceleration;

	s->bFriendlyFire = this->FriendlyFire * 1000.0f;
	s->fFriendlyFireDamageMultiplier = this->FriendlyFireMultiplier;
	s->BaseDestructionLimit = this->BaseDestructionKickLimit;
	s->FFDamageLimit = this->FriendlyFireDamageKickLimit;
	s->FFKillLimit = this->FriendlyFireKillKickLimit;

	s->fEnergyMultiplier = this->EnergyMultiplier * 1000.0f;
	s->fAoESizeMultiplier = this->AoESizeMultiplier * 1000.0f;
	s->fAoEDamageMultiplier = this->AoEDamageMultiplier * 1000.0f;

	// Verify how this indexing works now...
	s->ClassCounts[0] = this->LightCountLimit;
	s->ClassCounts[1] = this->MediumCountLimit;
	s->ClassCounts[2] = this->HeavyCountLimit;

	s->ArenaRounds = this->ArenaRounds;
	s->GameScores[TGT_CTF] = this->CTFCapLimit;
	s->GameScores[TGT_TDM] = this->TDMKillLimit;
	s->GameScores[TGT_ARN] = this->ArenaLives;
	s->GameScores[TGT_RAB] = this->RabbitScoreLimit;
	s->GameScores[TGT_CAH] = this->CaHScoreLimit;

	s->fVehicleHealthMultiplier = this->VehicleHealthMultiplier * 1000.0f;
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
	Logger::debug("[LoadServerSettings]");
	g_config.serverSettings.ApplyToGame(that);
	that->ApplyServerSettings();
}

void TrPlayerController_GetRespawnDelayTotalTime(ATrPlayerController* that, ATrPlayerController_execGetRespawnDelayTotalTime_Parms* params, float* result, Hooks::CallInfo* callInfo) {
	*result = that->GetRespawnDelayTotalTime();

	for (int i = EQP_Primary; i <= EQP_Quaternary; ++i) {
		int item = ((ATrPlayerReplicationInfo*)(that->PlayerReplicationInfo))->r_EquipLevels[i].EquipId;
		if (item == CONST_WEAPON_ID_RIFLE_SNIPER || item == CONST_WEAPON_ID_RIFLE_SNIPER_MKD || item == CONST_WEAPON_ID_RIFLE_PHASE || item == CONST_WEAPON_ID_SAP20) {
			// Player has a sniper rifle, if there's an extra sniper spawn delay, add it on
			*result += that->fSniperRespawnDelay;
			break;
		}
	}
}

// Map codes
static int mapCodeCTFKatabatic = CONST_MAP_ID_CTF_KATABATIC;
static int mapCodeCTFArxNovena = CONST_MAP_ID_CTF_ARXNOVENA;
static int mapCodeCTFDangerousCrossing = CONST_MAP_ID_CTF_DANGEROUSCROSSING;
static int mapCodeCTFDrydock = CONST_MAP_ID_CTF_DRYDOCK;
static int mapCodeCTFCrossfire = CONST_MAP_ID_CTF_CROSSFIRE;
static int mapCodeCTFPermafrost = CONST_MAP_ID_CTF_PERMAFROST;
static int mapCodeCTFTartarus = CONST_MAP_ID_CTF_TARTARUS;
static int mapCodeCTFCCR = CONST_MAP_ID_CTF_CANYONCRUSADEREVIVAL;
static int mapCodeCTFRaindance = CONST_MAP_ID_CTF_RAINDANCE;
static int mapCodeCTFTempleRuins = CONST_MAP_ID_CTF_TEMPLERUINS;
static int mapCodeCTFStonehenge = CONST_MAP_ID_CTF_STONEHENGE;
static int mapCodeCTFSunstar = CONST_MAP_ID_CTF_SUNSTAR;
static int mapCodeCTFBellaOmega = CONST_MAP_ID_CTF_BELLAOMEGA;
static int mapCodeCTFBellaOmegaNS = CONST_MAP_ID_CTF_BELLAOMEGANS;
static int mapCodeCTFTerminus = CONST_MAP_ID_CTF_TERMINUS;
static int mapCodeCTFIceCoaster = CONST_MAP_ID_CTF_ICECOASTER;
static int mapCodeCTFPerdition = CONST_MAP_ID_CTF_PERDITION;
static int mapCodeCTFHellfire = CONST_MAP_ID_CTF_HELLFIRE;
static int mapCodeCTFBlueshift = CONST_MAP_ID_CTF_BLUESHIFT;

static int mapCodeRabbitOutskirts = CONST_MAP_ID_RABBIT_OUTSKIRTS;
static int mapCodeRabbitQuicksand = CONST_MAP_ID_RABBIT_QUICKSAND;
static int mapCodeRabbitCrossfire = CONST_MAP_ID_RABBIT_CROSSFIRE;
static int mapCodeRabbitInferno = CONST_MAP_ID_RABBIT_INFERNO;
static int mapCodeRabbitNightabatic = CONST_MAP_ID_RABBIT_NIGHTABATIC;
static int mapCodeRabbitSulfurCove = CONST_MAP_ID_RABBIT_SULFURCOVE;

static int mapCodeTDMDrydockNight = CONST_MAP_ID_TDM_DRYDOCKNIGHT;
static int mapCodeTDMCrossfire = CONST_MAP_ID_TDM_CROSSFIRE;
static int mapCodeTDMQuicksand = CONST_MAP_ID_TDM_QUICKSAND;
static int mapCodeTDMNightabatic = CONST_MAP_ID_TDM_NIGHTABATIC;
static int mapCodeTDMInferno = CONST_MAP_ID_TDM_INFERNO;
static int mapCodeTDMSulfurCove = CONST_MAP_ID_TDM_SULFURCOVE;
static int mapCodeTDMOutskirts = CONST_MAP_ID_TDM_OUTSKIRTS;
static int mapCodeTDMMiasma = CONST_MAP_ID_TDM_MIASMA;
static int mapCodeTDMPerdition = CONST_MAP_ID_TDM_PERDITION;

static int mapCodeArenaAirArena = CONST_MAP_ID_ARENA_AIRARENA;
static int mapCodeArenaWalledIn = CONST_MAP_ID_ARENA_WALLEDIN;
static int mapCodeArenaLavaArena = CONST_MAP_ID_ARENA_LAVAARENA;
static int mapCodeArenaHinterlands = CONST_MAP_ID_ARENA_HINTERLANDS;
static int mapCodeArenaWhiteOut = CONST_MAP_ID_ARENA_WHITEOUT;
static int mapCodeArenaFraytown = CONST_MAP_ID_ARENA_FRAYTOWN;
static int mapCodeArenaUndercroft = CONST_MAP_ID_ARENA_UNDERCROFT;

static int mapCodeCaHKatabatic = CONST_MAP_ID_CAH_KATABATIC;
static int mapCodeCaHOutskirts = CONST_MAP_ID_CAH_OUTSKIRTS3P;
static int mapCodeCaHRaindance = CONST_MAP_ID_CAH_RAINDANCE;
static int mapCodeCaHDrydockNight = CONST_MAP_ID_CAH_DRYDOCKNIGHT;
static int mapCodeCaHSulfurCove = CONST_MAP_ID_CAH_SULFURCOVE;
static int mapCodeCaHTartarus = CONST_MAP_ID_CAH_TARTARUS;
static int mapCodeCaHCCR = CONST_MAP_ID_CAH_CANYONCRUSADEREVIVAL;

static int mapCodeBlitzCrossfire = CONST_MAP_ID_BLITZ_CROSSFIRE;
static int mapCodeBlitzCCR = CONST_MAP_ID_BLITZ_CANYONCRUSADEREVIVAL;
static int mapCodeBlitzBellaOmega = CONST_MAP_ID_BLITZ_BELLAOMEGA;
static int mapCodeBlitzBlueshift = CONST_MAP_ID_BLITZ_BLUESHIFT;
static int mapCodeBlitzHellfire = CONST_MAP_ID_BLITZ_HELLFIRE;
static int mapCodeBlitzArxNovena = CONST_MAP_ID_BLITZ_ARXNOVENA;
static int mapCodeBlitzKatabatic = CONST_MAP_ID_BLITZ_KATABATIC;
static int mapCodeBlitzDrydock = CONST_MAP_ID_BLITZ_DRYDOCK;
static int mapCodeBlitzIceCoaster = CONST_MAP_ID_BLITZ_ICECOASTER;
static int mapCodeBlitzTerminus = CONST_MAP_ID_BLITZ_TERMINUS;

static int mapRotationModeSequential = (int)MapRotationMode::SEQUENTIAL;
static int mapRotationModeRandom = (int)MapRotationMode::RANDOM;

static int teamAssignTypeBalanced = (int)TeamAssignTypes::TAT_BALANCED;
static int teamAssignTypeUnbalanced = (int)TeamAssignTypes::TAT_UNBALANCED;
static int teamAssignTypeAutoAssign = (int)TeamAssignTypes::TAT_AUTOASSIGN;

// Nasty nasty getter/setter generation
#define SETTING_GETTERSETTER(type, var) static type get ## var ##() { \
	return g_config.serverSettings.var; \
} \
static void set ## var ##(type n) { \
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

// Server setting getter/setters
SETTING_GETTERSETTER(int, TimeLimit)
SETTING_GETTERSETTER(int, WarmupTime)
SETTING_GETTERSETTER(int, OvertimeLimit)
SETTING_GETTERSETTER(int, RespawnTime)
SETTING_GETTERSETTER(int, SniperRespawnDelay)

SETTING_GETTERSETTER(int, MaxPlayers)
SETTING_GETTERSETTER_CAST(int, TeamAssignTypes, TeamAssignType)
SETTING_GETTERSETTER(bool, NakedSpawn)
SETTING_GETTERSETTER(bool, AutoBalanceTeams)

SETTING_GETTERSETTER(int, FlagDragLight)
SETTING_GETTERSETTER(int, FlagDragMedium)
SETTING_GETTERSETTER(int, FlagDragHeavy)
SETTING_GETTERSETTER(int, FlagDragDeceleration)

SETTING_GETTERSETTER(bool, FriendlyFire)
SETTING_GETTERSETTER(float, FriendlyFireMultiplier)
SETTING_GETTERSETTER(int, BaseDestructionKickLimit)
SETTING_GETTERSETTER(int, FriendlyFireDamageKickLimit)
SETTING_GETTERSETTER(int, FriendlyFireKillKickLimit)

SETTING_GETTERSETTER(float, EnergyMultiplier)
SETTING_GETTERSETTER(float, AoESizeMultiplier)
SETTING_GETTERSETTER(float, AoEDamageMultiplier)

SETTING_GETTERSETTER(int, LightCountLimit)
SETTING_GETTERSETTER(int, MediumCountLimit)
SETTING_GETTERSETTER(int, HeavyCountLimit)

SETTING_GETTERSETTER(int, ArenaRounds)
SETTING_GETTERSETTER(int, CTFCapLimit)
SETTING_GETTERSETTER(int, TDMKillLimit)
SETTING_GETTERSETTER(int, ArenaLives)
SETTING_GETTERSETTER(int, RabbitScoreLimit)
SETTING_GETTERSETTER(int, CaHScoreLimit)

SETTING_GETTERSETTER(float, VehicleHealthMultiplier)
SETTING_GETTERSETTER(int, GravCycleLimit)
SETTING_GETTERSETTER(int, BeowulfLimit)
SETTING_GETTERSETTER(int, ShrikeLimit)
SETTING_GETTERSETTER(int, GravCycleSpawnTime)
SETTING_GETTERSETTER(int, BeowulfSpawnTime)
SETTING_GETTERSETTER(int, ShrikeSpawnTime)

SETTING_GETTERSETTER(bool, BaseAssets)
SETTING_GETTERSETTER(bool, BaseUpgrades)
SETTING_GETTERSETTER(bool, PoweredDeployables)
SETTING_GETTERSETTER(bool, GeneratorRegen)
SETTING_GETTERSETTER(bool, GeneratorDestroyable)
SETTING_GETTERSETTER(bool, BaseAssetFriendlyFire)
SETTING_GETTERSETTER(bool, DeployableFriendlyFire)

SETTING_GETTERSETTER(bool, SkiingEnabled)
SETTING_GETTERSETTER(bool, CTFBlitzAllFlagsMove)
SETTING_GETTERSETTER(bool, ForceHardcodedLoadouts)

static void addCustomToMapRotation(std::string mapName) {
	g_config.serverSettings.mapRotation.push_back(mapName);
}

static void addToMapRotation(int mapId) {
	auto& it = Data::map_id_to_filename.find(mapId);
	if (it == Data::map_id_to_filename.end()) {
		Logger::error("Failed to add map %d to rotation; no such map exists", mapId);
		return;
	}
	g_config.serverSettings.mapRotation.push_back(it->second);
}

static void addToBannedItemsList(std::string className, std::string itemName) {
	int item = Data::getItemId(className, itemName);
	if (item) {
		g_config.serverSettings.bannedItems.insert(item);
		Logger::debug("Added ban for item %d", item);
	}
}

static void removeFromBannedItemsList(std::string className, std::string itemName) {
	int item = Data::getItemId(className, itemName);
	if (item) {
		g_config.serverSettings.bannedItems.erase(item);
	}
}

static void addToDisabledEquipPointsList(std::string className, int eqp) {
	int classNum = Utils::searchMapId(Data::classes, className, "", false) - 1;
	if (classNum == -1) return;
	int classId = Data::classes_id[classNum];

	switch (classId) {
	case CONST_CLASS_TYPE_LIGHT:
		g_config.serverSettings.disabledEquipPointsLight.insert(eqp);
		break;
	case CONST_CLASS_TYPE_MEDIUM:
		g_config.serverSettings.disabledEquipPointsMedium.insert(eqp);
		break;
	case CONST_CLASS_TYPE_HEAVY:
		g_config.serverSettings.disabledEquipPointsHeavy.insert(eqp);
		break;
	}
}

static void removeFromDisabledEquipPointsList(std::string className, int eqp) {
	int classNum = Utils::searchMapId(Data::classes, className, "", false) - 1;
	if (classNum == -1) return;
	int classId = Data::classes_id[classNum];

	switch (classId) {
	case CONST_CLASS_TYPE_LIGHT:
		g_config.serverSettings.disabledEquipPointsLight.erase(eqp);
		break;
	case CONST_CLASS_TYPE_MEDIUM:
		g_config.serverSettings.disabledEquipPointsMedium.erase(eqp);
		break;
	case CONST_CLASS_TYPE_HEAVY:
		g_config.serverSettings.disabledEquipPointsHeavy.erase(eqp);
		break;
	}
}

static void addToBannedItemsListNoClass(std::string itemName) {
	addToBannedItemsList("Light", itemName);
}

static void removeFromBannedItemsListNoClass(std::string itemName) {
	removeFromBannedItemsList("Light", itemName);
}

namespace LuaAPI {
	void addServerSettingsAPI(luabridge::Namespace ns) {
		ns
			.beginNamespace("ServerSettings")
				.SETTING_LUAPROP(TimeLimit)
				.SETTING_LUAPROP(WarmupTime)
				.SETTING_LUAPROP(OvertimeLimit)
				.SETTING_LUAPROP(RespawnTime)
				.SETTING_LUAPROP(SniperRespawnDelay)

				.SETTING_LUAPROP(MaxPlayers)
				.SETTING_LUAPROP(TeamAssignType)
				.SETTING_LUAPROP(NakedSpawn)
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
				.SETTING_LUAPROP(ForceHardcodedLoadouts)
				.beginNamespace("MapRotation")
					.addVariable("Mode", (int*)&g_config.serverSettings.mapRotationMode, true)
					.beginNamespace("Modes")
						.addVariable("Sequential", &mapRotationModeSequential, false)
						.addVariable("Random", &mapRotationModeRandom, false)
					.endNamespace()
					.addFunction("add", &addToMapRotation)
					.addFunction("addCustom", &addCustomToMapRotation)
				.endNamespace()
			.beginNamespace("BannedItems")
				.addFunction("add", &addToBannedItemsList)
				.addFunction("remove", &removeFromBannedItemsList)
			.endNamespace()
			.beginNamespace("DisabledEquipPoints")
				.addFunction("add", &addToDisabledEquipPointsList)
				.addFunction("remove", &removeFromDisabledEquipPointsList)
			.endNamespace()
			.endNamespace()
			.beginNamespace("TeamAssignTypes")
				.addVariable("Balanced", &teamAssignTypeBalanced, false)
				.addVariable("Unbalanced", &teamAssignTypeUnbalanced, false)
				.addVariable("AutoAssign", &teamAssignTypeAutoAssign, false)
			.endNamespace()
			.beginNamespace("Maps")
				.beginNamespace("CTF")
					.addVariable("Katabatic", &mapCodeCTFKatabatic, false)
					.addVariable("Kata", &mapCodeCTFKatabatic, false)
					.addVariable("ArxNovena", &mapCodeCTFArxNovena, false)
					.addVariable("Arx", &mapCodeCTFArxNovena, false)
					.addVariable("DangerousCrossing", &mapCodeCTFDangerousCrossing, false)
					.addVariable("DX", &mapCodeCTFDangerousCrossing, false)
					.addVariable("Drydock", &mapCodeCTFDrydock, false)
					.addVariable("DD", &mapCodeCTFDrydock, false)
					.addVariable("Crossfire", &mapCodeCTFCrossfire, false)
					.addVariable("XF", &mapCodeCTFCrossfire, false)
					.addVariable("XFire", &mapCodeCTFCrossfire, false)
					.addVariable("Permafrost", &mapCodeCTFPermafrost, false)
					.addVariable("Tartarus", &mapCodeCTFTartarus, false)
					.addVariable("CCR", &mapCodeCTFCCR, false)
					.addVariable("CanyonCrusade", &mapCodeCTFCCR, false)
					.addVariable("CanyonCrusadeRevival", &mapCodeCTFCCR, false)
					.addVariable("Raindance", &mapCodeCTFRaindance, false)
					.addVariable("TempleRuins", &mapCodeCTFTempleRuins, false)
					.addVariable("Harabec", &mapCodeCTFTempleRuins, false)
					.addVariable("RuinsOfHarabec", &mapCodeCTFTempleRuins, false)
					.addVariable("Temple", &mapCodeCTFTempleRuins, false)
					.addVariable("Stonehenge", &mapCodeCTFStonehenge, false)
					.addVariable("Sunstar", &mapCodeCTFSunstar, false)
					.addVariable("BellaOmega", &mapCodeCTFBellaOmegaNS, false)
					.addVariable("Bella", &mapCodeCTFBellaOmegaNS, false)
					.addVariable("BellaOmegaNS", &mapCodeCTFBellaOmegaNS, false)
					.addVariable("BellaOmegaSandstorm", &mapCodeCTFBellaOmega, false)
					.addVariable("Terminus", &mapCodeCTFTerminus, false)
					.addVariable("IceCoaster", &mapCodeCTFIceCoaster, false)
					.addVariable("Perdition", &mapCodeCTFPerdition, false)
					.addVariable("Hellfire", &mapCodeCTFHellfire, false)
					.addVariable("Blueshift", &mapCodeCTFBlueshift, false)
				.endNamespace()
				.beginNamespace("TDM")
					.addVariable("DrydockNight", &mapCodeTDMDrydockNight, false)
					.addVariable("Crossfire", &mapCodeTDMCrossfire, false)
					.addVariable("XF", &mapCodeTDMCrossfire, false)
					.addVariable("XFire", &mapCodeTDMCrossfire, false)
					.addVariable("Quicksand", &mapCodeTDMQuicksand, false)
					.addVariable("Nightabatic", &mapCodeTDMNightabatic, false)
					.addVariable("Inferno", &mapCodeTDMInferno, false)
					.addVariable("SulfurCove", &mapCodeTDMSulfurCove, false)
					.addVariable("Sulfur", &mapCodeTDMSulfurCove, false)
					.addVariable("Outskirts", &mapCodeTDMOutskirts, false)
					.addVariable("Outskirts3P", &mapCodeTDMOutskirts, false)
					.addVariable("Miasma", &mapCodeTDMMiasma, false)
					.addVariable("Perdition", &mapCodeTDMPerdition, false)
				.endNamespace()
				.beginNamespace("Arena")
					.addVariable("AirArena", &mapCodeArenaAirArena, false)
					.addVariable("Air", &mapCodeArenaAirArena, false)
					.addVariable("WalledIn", &mapCodeArenaWalledIn, false)
					.addVariable("Walledin", &mapCodeArenaWalledIn, false)
					.addVariable("LavaArena", &mapCodeArenaLavaArena, false)
					.addVariable("Lava", &mapCodeArenaLavaArena, false)
					.addVariable("Hinterlands", &mapCodeArenaHinterlands, false)
					.addVariable("WhiteOut", &mapCodeArenaWhiteOut, false)
					.addVariable("Whiteout", &mapCodeArenaWhiteOut, false)
					.addVariable("FrayTown", &mapCodeArenaFraytown, false)
					.addVariable("Fraytown", &mapCodeArenaFraytown, false)
					.addVariable("Undercroft", &mapCodeArenaUndercroft, false)
				.endNamespace()
				.beginNamespace("Rabbit")
					.addVariable("Outskirts", &mapCodeRabbitOutskirts, false)
					.addVariable("Outskirts3P", &mapCodeRabbitOutskirts, false)
					.addVariable("Quicksand", &mapCodeRabbitQuicksand, false)
					.addVariable("Crossfire", &mapCodeRabbitCrossfire, false)
					.addVariable("XF", &mapCodeRabbitCrossfire, false)
					.addVariable("XFire", &mapCodeRabbitCrossfire, false)
					.addVariable("Inferno", &mapCodeRabbitInferno, false)
					.addVariable("Nightabatic", &mapCodeRabbitNightabatic, false)
					.addVariable("SulfurCove", &mapCodeRabbitSulfurCove, false)
					.addVariable("Sulfur", &mapCodeRabbitSulfurCove, false)
				.endNamespace()
				.beginNamespace("CaH")
					.addVariable("Katabatic", &mapCodeCaHKatabatic, false)
					.addVariable("Kata", &mapCodeCaHKatabatic, false)
					.addVariable("Outskirts", &mapCodeCaHOutskirts, false)
					.addVariable("Outskirts3P", &mapCodeCaHOutskirts, false)
					.addVariable("Raindance", &mapCodeCaHRaindance, false)
					.addVariable("DrydockNight", &mapCodeCaHDrydockNight, false)
					.addVariable("SulfurCove", &mapCodeCaHSulfurCove, false)
					.addVariable("Tartarus", &mapCodeCaHTartarus, false)
					.addVariable("CCR", &mapCodeCaHCCR, false)
					.addVariable("CanyonCrusade", &mapCodeCaHCCR, false)
					.addVariable("CanyonCrusadeRevival", &mapCodeCaHCCR, false)
				.endNamespace()
				.beginNamespace("Blitz")
					.addVariable("Crossfire", &mapCodeBlitzCrossfire, false)
					.addVariable("XF", &mapCodeBlitzCrossfire, false)
					.addVariable("XFire", &mapCodeBlitzCrossfire, false)
					.addVariable("CCR", &mapCodeBlitzCCR, false)
					.addVariable("CanyonCrusade", &mapCodeBlitzCCR, false)
					.addVariable("CanyonCrusadeRevival", &mapCodeBlitzCCR, false)
					.addVariable("BellaOmega", &mapCodeBlitzBellaOmega, false)
					.addVariable("Bella", &mapCodeBlitzBellaOmega, false)
					.addVariable("Blueshift", &mapCodeBlitzBlueshift, false)
					.addVariable("Hellfire", &mapCodeBlitzHellfire, false)
					.addVariable("ArxNovena", &mapCodeBlitzArxNovena, false)
					.addVariable("Arx", &mapCodeBlitzArxNovena, false)
					.addVariable("Katabatic", &mapCodeBlitzKatabatic, false)
					.addVariable("Kata", &mapCodeBlitzKatabatic, false)
					.addVariable("Drydock", &mapCodeBlitzDrydock, false)
					.addVariable("DD", &mapCodeBlitzDrydock, false)
					.addVariable("IceCoaster", &mapCodeBlitzIceCoaster, false)
					.addVariable("Terminus", &mapCodeBlitzTerminus, false)
				.endNamespace()
			.endNamespace()
			;
	}
}