#include "Mods.h"

// Sneaky state used to force a map switch for initialisation
// With a timer so that things can process, to avoid race conditions during init
// 5 ticks seems to do the trick
static int changeMapTickCounter = -1;
static TAServer::PersistentContext controllerContext;

static void checkForScoreChange() {
	static int beScoreCached = 0;
	static int dsScoreCached = 0;
	int beScoreCurrent = (int)Utils::tr_gri->Teams.Data[0]->Score;
	int dsScoreCurrent = (int)Utils::tr_gri->Teams.Data[1]->Score;
	if (beScoreCurrent != beScoreCached || dsScoreCurrent != dsScoreCached) {
		beScoreCached = beScoreCurrent;
		dsScoreCached = dsScoreCurrent;

		if (g_config.connectToTAServer && g_TAServerClient.isConnected()) {
			g_TAServerClient.sendScoreInfo(beScoreCurrent, dsScoreCurrent);
		}
	}
	else {
	}
}

static void updateMatchTime(bool counting) {
	if (g_config.connectToTAServer && g_TAServerClient.isConnected() && Utils::tr_gri) {
		int timeLeft = Utils::tr_gri->TimeLimit != 0 ? Utils::tr_gri->RemainingTime : Utils::tr_gri->ElapsedTime;
		g_TAServerClient.sendMatchTime(timeLeft, counting);
	}
}

void TrGame_RequestTeam(ATrGame* that, ATrGame_execRequestTeam_Parms* params, bool* result, Hooks::CallInfo* callInfo) {
	// Perform authorization if we aren't allowing unmodded clients
	// Don't need to authorize to join spec
	if (params->RequestedTeamNum != 255 && g_config.connectToClients && !g_config.allowUnmoddedClients) {
		if (!params->C || !params->C->PlayerReplicationInfo) {
			// Failed to get PRI, don't allow
			Logger::error("Blocked a player from joining a team, unable to get their PRI");
			return;
		}

		if (!g_DCServer.isPlayerAKnownModdedConnection(params->C->PlayerReplicationInfo->UniqueId)) {
			// Player hasn't connected to the server, don't allow
			Logger::warn("Player with ID %d was blocked from joining a team as they are not a known connection");
			return;
		}
	}

	*result = that->RequestTeam(params->RequestedTeamNum, params->C);
	if (params->C->PlayerReplicationInfo) {
		Logger::debug("Requesting team from PlayerID %d", params->C->PlayerReplicationInfo->UniqueId);
	}
	if (!*result) return;

	if (g_config.connectToTAServer && g_TAServerClient.isConnected()) {
		// Send team info
		ATrGameReplicationInfo* gri = (ATrGameReplicationInfo*)that->WorldInfo->Game->GameReplicationInfo;
		if (!gri) return;

		std::map<long long, int> playerToTeamId;
		for (int i = 0; i < gri->PRIArray.Count; ++i) {
			ATrPlayerReplicationInfo* pri = (ATrPlayerReplicationInfo*)gri->PRIArray.GetStd(i);
			if (!pri) continue;
			// There's always an extra 'player' with ID 0 connected; this is used by the real login servers to scrape info (we think)
			// not useful here, ignore it
			if (TAServer::netIdToLong(pri->UniqueId) == 0) continue;
			playerToTeamId[TAServer::netIdToLong(pri->UniqueId)] = pri->GetTeamNum();
		}

		g_TAServerClient.sendTeamInfo(playerToTeamId);
	}
}

bool UTGame_MatchInProgress_BeginState(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {
	Utils::serverGameStatus = Utils::ServerGameStatus::IN_PROGRESS;
	if (g_config.connectToTAServer && g_TAServerClient.isConnected() && Utils::tr_gri) {
		{
			std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
			g_TAServerClient.sendMatchTime(Utils::tr_gri->r_ServerConfig->TimeLimit * 60, true);
		}
		
	}
	return false;
}

static int getNextMapIdx() {
	if (bNextMapOverrideValue != 0 && Data::map_id_to_filename.find(bNextMapOverrideValue) != Data::map_id_to_filename.end()) {
		// Override for next map has been set
		// Shouldn
		return bNextMapOverrideValue;
	}
	else if (g_config.serverSettings.mapRotationMode == MapRotationMode::RANDOM) {
		std::random_device rd;
		std::mt19937 randgen(rd());
		std::uniform_int_distribution<> rand_dist(0, g_config.serverSettings.mapRotation.size());
		return rand_dist(randgen);
	}
	else {
		return (g_config.serverSettings.mapRotationIndex + 1) % g_config.serverSettings.mapRotation.size();
	}
}

static std::string getNextMapName() {
	std::string nextMapName;
	if (bNextMapOverrideValue != 0 && Data::map_id_to_filename.find(bNextMapOverrideValue) != Data::map_id_to_filename.end()) {
		// Override for next map has been set
		nextMapName = Data::map_id_to_filename[bNextMapOverrideValue];
		bNextMapOverrideValue = 0;
	}
	else {
		nextMapName = g_config.serverSettings.mapRotation[getNextMapIdx()];
	}

	return nextMapName;
}

void UTGame_EndGame(AUTGame* that, AUTGame_execEndGame_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	Utils::serverGameStatus = Utils::ServerGameStatus::ENDED;
	that->EndGame(params->Winner, params->Reason);

	// Tell TAServer that the game is ending
	if (g_config.connectToTAServer && g_TAServerClient.isConnected()) {
		{
			std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
			g_TAServerClient.sendMatchTime(0, false);
		}

		// If a map override has been issued, we need to explicitly give the map name
		std::string nextMapOverrideName = bNextMapOverrideValue == 0 ? "" : getNextMapName();
		
		g_TAServerClient.sendMatchEnded(getNextMapIdx(), nextMapOverrideName);
	}
}

void TAServer::Client::handler_Launcher2GameInitMessage(const json& msgBody) {
	// Received when the launcher is ready for us to initialise
	Launcher2GameInitMessage msg;
	if (!msg.fromJson(msgBody)) {
		Logger::fatal("Failed to parse game server initialisation message: %s", msgBody.dump().c_str());
		return;
	}

	// Set up the controller context
	controllerContext = msg.controllerContext;
	g_config.serverSettings.mapRotationIndex = controllerContext.nextMapIndex;

	if (controllerContext.hasContext) {
		Logger::info("Received controller context; switching to rotation index %d; map override \"%s\"", controllerContext.nextMapIndex, controllerContext.nextMapOverride);
	}
	else {
		Logger::info("Received empty controller context, starting map rotation");
	}
	

	// And in 5 ticks, perform the map switch
	changeMapTickCounter = 5;
}

void UTGame_ProcessServerTravel(AUTGame* that, AUTGame_execProcessServerTravel_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	// Invalidate the global GRI
	{
		std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
		// Ensure latest data has been sent to the login server
		checkForScoreChange();
		// Invalidate the GRI
		Utils::tr_gri = NULL;
	}
	Logger::debug("About to perform ProcessServerTravel");
	that->ProcessServerTravel(params->URL, params->bAbsolute);
	Logger::debug("Done ProcessServerTravel");
}

void TrGame_ApplyServerSettings(ATrGame* that, ATrGame_execApplyServerSettings_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	that->ApplyServerSettings();
}

static bool shouldRemoveAllPlayersFromTeamsOnNextTick = false;

bool TrGameReplicationInfo_PostBeginPlay(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {
	{
		Utils::serverGameStatus = Utils::ServerGameStatus::PREROUND;
		std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);

		Utils::tr_gri = (ATrGameReplicationInfo*)dwCallingObject;

		if (g_config.connectToTAServer && g_TAServerClient.isConnected()) {
			// GRI should only be set at the start of the match -> ergo not counting down
			g_TAServerClient.sendMatchTime(Utils::tr_gri->r_ServerConfig->TimeLimit * 60, false);
			Utils::tr_gri->RemainingMinute = Utils::tr_gri->r_ServerConfig->TimeLimit * 60;
		}
	}

	shouldRemoveAllPlayersFromTeamsOnNextTick = true;

	return false;
}

void TrGame_SendShowSummary(ATrGame* that, ATrGame_execSendShowSummary_Parms* params, void* result, Hooks::CallInfo callInfo) {
	// Seeing as we don't support the match end screen anyway, this can be blackholed
	// It causes the 'getting stuck in the menu glitch'
}

static bool startedDCServer = false;

bool TrGameReplicationInfo_Tick(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {

	ATrGameReplicationInfo* that = (ATrGameReplicationInfo*)dwCallingObject;

	// If we haven't already, start the Direct Connection server
	if (!startedDCServer && g_config.connectToClients && that->WorldInfo) {
		std::string url = Utils::f2std(that->WorldInfo->GetAddressURL());

		int port = 7777;
		size_t portPos = url.rfind(':');
		if (portPos != std::string::npos) {
			try {
				// Server will be connected through UDP proxy
				// We subtract 100 from the proxied port to get the 'real' UDP port clients are connecting to
				// and run the DC Server on the TCP port equivalent to that UDP port
				port = std::stoi(url.substr(portPos + 1)) - 100;
			}
			catch (std::invalid_argument&) {}
		}

		Logger::info("Starting DC Server on TCP port %d..", port);
		g_DCServer.start(port);
		startedDCServer = true;
	}

	if (Utils::serverGameStatus == Utils::ServerGameStatus::PREROUND) {
		{
			std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
			if (Utils::tr_gri) {
				Utils::tr_gri->RemainingTime = 60 * Utils::tr_gri->r_ServerConfig->TimeLimit;
				Utils::tr_gri->RemainingMinute = 60 * Utils::tr_gri->r_ServerConfig->TimeLimit;
			}
		}
	}

	if (shouldRemoveAllPlayersFromTeamsOnNextTick) {
		shouldRemoveAllPlayersFromTeamsOnNextTick = false;
		for (int i = 0; i < Utils::tr_gri->PRIArray.Count; ++i) {
			ATrPlayerReplicationInfo* pri = (ATrPlayerReplicationInfo*)Utils::tr_gri->PRIArray.GetStd(i);
			if (pri->Team && pri->Owner) {
				ATrPlayerController* pc = (ATrPlayerController*)pri->Owner;
				pri->Team->RemoveFromTeam(pc);
				pc->r_bNeedLoadout = true;
				pc->r_bNeedTeam = true;

			}
			else {
			}
		}

		// Re-send game balance info to all players
		// This lets us mutate game balance between maps
		g_DCServer.forAllKnownConnections([](DCServer::Server* srv, std::shared_ptr<DCServer::PlayerConnection> pconn) {
			// Send the player the current balance state
			srv->sendGameBalanceDetailsMessage(pconn,
				g_config.getReplicatedSettings(),
				g_config.serverSettings.weaponProperties,
				g_config.serverSettings.deviceValueProperties,
				g_config.serverSettings.classProperties,
				g_config.serverSettings.vehicleProperties,
				g_config.serverSettings.vehicleWeaponProperties
			);
		});

	}
	

	if (changeMapTickCounter > 0) changeMapTickCounter--;
	if (changeMapTickCounter == 0) {
		Utils::tr_gri = that;
		changeMapTickCounter = -1;
		json j;
		g_TAServerClient.handler_Launcher2GameNextMapMessage(j);
	}
	return false;
}

void TAServer::Client::handler_OnConnect() {
	// Immediately send TAServer the protocol version
	g_TAServerClient.sendProtocolVersion();

	// Below is commented out for the new blue-green dual server approach to map switches
	// Instead, we will wait until the launcher sends its initialisation message before starting up

	// Force a change map message to immediately go to the first map of the rotation
	// This also forces a reload of the server settings
	//changeMapTickCounter = 5;
}

static void performMapChange(std::string mapName) {
	Logger::info("Changing map to %s...", mapName.c_str());
	std::wstring wideMapName(mapName.begin(), mapName.end());
	wchar_t* mapNameData = new wchar_t[wideMapName.size() + 1];
	wcscpy(mapNameData, wideMapName.c_str());

	Utils::tr_gri->WorldInfo->eventServerTravel(FString(mapNameData), false, false);

	delete mapNameData;
}

void TAServer::Client::handler_Launcher2GameNextMapMessage(const json& msgBody) {
	// This is actually called from two places:
	// 1) handler for the next map message (only relevant if using SeamlessTravel switching, rather than blue-green dual server)
	// 2) When the initialisation map tick counter hits zero (relevant regardless of switching method)

	if (!g_config.connectToTAServer || !g_TAServerClient.isConnected()) return;

	//std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
	if (!Utils::tr_gri || !Utils::tr_gri->WorldInfo) return;

	// Reset team scores
	for (int i = 0; i < Utils::tr_gri->Teams.Count; ++i) {
		Utils::tr_gri->Teams.Data[i]->Score = 0;
	}

	// Reset call-in laser target buildup/cooldown cache
	ResetLaserTargetCallInCache();

	std::string nextMapName;
	if (g_config.serverSettings.mapRotation.empty()) {
		//Utils::tr_gri->WorldInfo->eventServerTravel(FString(L"?restart"), false, false);
		nextMapName = "?restart";
	}
	else {
		if (controllerContext.hasContext) {
			// We have context from the launcher, choose the map to switch to from that

			// Do we have an override in the context?
			if (controllerContext.nextMapOverride != "") {
				// Override, use that
				nextMapName = controllerContext.nextMapOverride;
			}
			else {
				// No override, use the rotation index
				if (controllerContext.nextMapIndex < 0 || controllerContext.nextMapIndex >= g_config.serverSettings.mapRotation.size()) {
					// Out of bounds, swap to the start of the rotation
					Logger::error("Context message rotation index %d is out of rotation bounds", controllerContext.nextMapIndex);
					nextMapName = g_config.serverSettings.mapRotation[0];
				}
				else {
					nextMapName = g_config.serverSettings.mapRotation[getNextMapIdx()];
				}
			}
		}
		else {
			// Assume we're at the start the rotation
			nextMapName = g_config.serverSettings.mapRotation[0];
			// Disabled - SeamlessTravel case, work it out
			//nextMapName = getNextMapName();
		}
	}

	

	// Tell the login server what the new map is
	// Reverse searching the mapid -> mapname because it's small and cbf'd using boost::bimap
	int mapId = -1;
	for (auto& it : Data::map_id_to_filename) {
		if (it.second == nextMapName) {
			mapId = it.first;
			break;
		}
	}
	// TODO: Fix this for the empty rotation case and custom map case
	g_TAServerClient.sendMapInfo(mapId == -1 ? 1456 : mapId);

	// Actually change map
	performMapChange(nextMapName);
}

void TAServer::Client::handler_Launcher2GamePingsMessage(const json& msgBody) {
	// Parse the message
	Launcher2GamePingsMessage msg;
	if (!msg.fromJson(msgBody)) {
		// Failed to parse
		Logger::error("Failed to parse player pings response: %s", msgBody.dump().c_str());
		return;
	}

	std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
	if (!Utils::tr_gri) return;

	for (int i = 0; i < Utils::tr_gri->PRIArray.Count; ++i) {
		ATrPlayerReplicationInfo* pri = (ATrPlayerReplicationInfo*)Utils::tr_gri->PRIArray.GetStd(i);
		long long playerId = TAServer::netIdToLong(pri->UniqueId);
		auto& it = msg.playerToPing.find(playerId);
		if (it == msg.playerToPing.end()) continue;

		pri->c_fCurrentPingMS = (float)it->second;
		pri->Ping = (it->second) / 4;
		pri->ExactPing = (it->second) / 4;
	}
}

// Blackholed
void PlayerController_ServerUpdatePing(APlayerController* that, APlayerController_execServerUpdatePing_Parms* params, void* result, Hooks::CallInfo* callInfo) {}

static ATrPlayerReplicationInfo* getPriForPlayerId(TArray<APlayerReplicationInfo*>& arr, long long id) {
	for (int i = 0; i < arr.Count; ++i) {
		if (i > arr.Count) break; // Mediocre attempt at dealing with concurrent modifications
		ATrPlayerReplicationInfo* curPRI = (ATrPlayerReplicationInfo*)arr.GetStd(i);
		long long cur = TAServer::netIdToLong(curPRI->UniqueId);
		if (cur == id) {
			return curPRI;
		}
	}

	return NULL;
}

static bool cachedWasInOvertime = false;
static int pollsSinceLastStateUpdate = 0;
void pollForGameInfoChanges() {

	// Lock the GRI to ensure it can't be unset during polling
	std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
	if (!Utils::tr_gri) {
		return;
	}
	
	// Score polling
	checkForScoreChange();

	// Ensure match time is updated when overtime is entered
	if (Utils::tr_gri->WorldInfo && Utils::tr_gri->WorldInfo->Game) {
		if (Utils::tr_gri->WorldInfo->Game->bOverTime != cachedWasInOvertime) {
			if (!cachedWasInOvertime) {
				updateMatchTime(true);
			}
			cachedWasInOvertime = Utils::tr_gri->WorldInfo->Game->bOverTime;
		}
	}


	// Send out player state every 10 seconds;
	if (pollsSinceLastStateUpdate >= 5) {
		// Send out to every player
		g_DCServer.forAllKnownConnections([](DCServer::Server* srv, std::shared_ptr<DCServer::PlayerConnection> pconn) {
			// Can assume tr_gri exists since we push player state during polling
			ATrPlayerReplicationInfo* pri = getPriForPlayerId(Utils::tr_gri->PRIArray, pconn->playerId);
			if (!pri) return;

			float ping = (pri->Ping * 4.0f) / (1000.0f);

			srv->sendStateUpdateMessage(pconn, ping);
		});
		pollsSinceLastStateUpdate = 0;
	}
	else {
		pollsSinceLastStateUpdate++;
	}
}