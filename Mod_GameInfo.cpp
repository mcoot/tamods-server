#include "Mods.h"

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

void UTGame_EndGame(AUTGame* that, AUTGame_execEndGame_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	Utils::serverGameStatus = Utils::ServerGameStatus::ENDED;
	that->EndGame(params->Winner, params->Reason);

	// Tell TAServer that the game is ending
	if (g_config.connectToTAServer && g_TAServerClient.isConnected()) {
		{
			std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
			g_TAServerClient.sendMatchTime(0, false);
		}
		
		g_TAServerClient.sendMatchEnded();
	}
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
	that->ProcessServerTravel(params->URL, params->bAbsolute);
}

static int changeMapTickCounter = -1;

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

bool TrGameReplicationInfo_Tick(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {

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
				Logger::debug("Need loadout? %d", pc->r_bNeedLoadout);
			}
			else {
			}
			
		}
	}
	

	if (changeMapTickCounter > 0) changeMapTickCounter--;
	if (changeMapTickCounter == 0) {
		Utils::tr_gri = (ATrGameReplicationInfo*)dwCallingObject;
		changeMapTickCounter = -1;
		json j;
		g_TAServerClient.handler_Launcher2GameNextMapMessage(j);
	}
	return false;
}

void TAServer::Client::handler_OnConnect() {
	// Immediately send TAServer the protocol version
	g_TAServerClient.sendProtocolVersion();

	// Force a change map message to immediately go to the first map of the rotation
	// This also forces a reload of the server settings
	changeMapTickCounter = 5;
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
	if (!g_config.connectToTAServer || !g_TAServerClient.isConnected()) return;

	//std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
	if (!Utils::tr_gri || !Utils::tr_gri->WorldInfo) return;

	if (g_config.serverSettings.mapRotation.empty()) {
		Utils::tr_gri->WorldInfo->eventServerTravel(FString(L"?restart"), false, false);
	}
	else {
		std::string nextMapName;
		if (bNextMapOverrideValue != 0 && Data::map_id_to_filename.find(bNextMapOverrideValue) != Data::map_id_to_filename.end()) {
			// Override for next map has been set
			nextMapName = Data::map_id_to_filename[bNextMapOverrideValue];
			bNextMapOverrideValue = 0;
		}
		else if (g_config.serverSettings.mapRotationMode == MapRotationMode::RANDOM) {
			std::random_device rd;
			std::mt19937 randgen(rd());
			std::uniform_int_distribution<> rand_dist(0, g_config.serverSettings.mapRotation.size());
			g_config.serverSettings.mapRotationIndex = rand_dist(randgen);
			nextMapName = g_config.serverSettings.mapRotation[g_config.serverSettings.mapRotationIndex];
		}
		else {
			g_config.serverSettings.mapRotationIndex = (g_config.serverSettings.mapRotationIndex + 1) % g_config.serverSettings.mapRotation.size();
			nextMapName = g_config.serverSettings.mapRotation[g_config.serverSettings.mapRotationIndex];
		}

		// Tell the login server what the new map is
		// Reverse searching the mapid -> mapname because it's small and cbf'd using boost::bimap
		for (auto& it : Data::map_id_to_filename) {
			if (it.second == nextMapName) {
				g_TAServerClient.sendMapInfo(it.first);
				break;
			}
		}

		// Actually change map
		performMapChange(nextMapName);
	}
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