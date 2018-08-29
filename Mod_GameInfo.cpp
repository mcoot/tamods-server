#include "Mods.h"

static void checkForScoreChange() {
	static int beScoreCached = 0;
	static int dsScoreCached = 0;
	int beScoreCurrent = (int)Utils::tr_gri->Teams.Data[0]->Score;
	int dsScoreCurrent = (int)Utils::tr_gri->Teams.Data[1]->Score;
	if (beScoreCurrent != beScoreCached || dsScoreCurrent != dsScoreCached) {
		Logger::debug("Polled: changed (%d, %d) -> (%d, %d)", beScoreCached, dsScoreCached, beScoreCurrent, dsScoreCurrent);
		beScoreCached = beScoreCurrent;
		dsScoreCached = dsScoreCurrent;

		if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected()) {
			g_TAServerClient.sendScoreInfo(beScoreCurrent, dsScoreCurrent);
		}
	}
	else {
	}
}

static void updateMatchTime(bool counting) {
	if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected() && Utils::tr_gri) {
		int timeLeft = Utils::tr_gri->TimeLimit != 0 ? Utils::tr_gri->RemainingTime : Utils::tr_gri->ElapsedTime;
		g_TAServerClient.sendMatchTime(timeLeft, counting);
	}
}

void TrGame_RequestTeam(ATrGame* that, ATrGame_execRequestTeam_Parms* params, bool* result, Hooks::CallInfo* callInfo) {
	*result = that->RequestTeam(params->RequestedTeamNum, params->C);
	if (!*result) return;

	if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected()) {
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
	Logger::debug("[Match start]");
	Utils::serverGameStatus = Utils::ServerGameStatus::IN_PROGRESS;
	if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected() && Utils::tr_gri) {
		{
			std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
			g_TAServerClient.sendMatchTime(Utils::tr_gri->r_ServerConfig->TimeLimit * 60, true);
		}
		
	}
	return false;
}

void UTGame_EndGame(AUTGame* that, AUTGame_execEndGame_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	Logger::debug("[UTGame.EndGame] called");
	Utils::serverGameStatus = Utils::ServerGameStatus::ENDED;
	that->EndGame(params->Winner, params->Reason);

	// Tell TAServer that the game is ending
	if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected()) {
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
		Logger::debug("Invalidating GRI");
		Utils::tr_gri = NULL;
	}
	that->ProcessServerTravel(params->URL, params->bAbsolute);
}

static int changeMapTickCounter = -1;

void TrGame_ApplyServerSettings(ATrGame* that, ATrGame_execApplyServerSettings_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	that->ApplyServerSettings();
	Logger::debug("[TrGame.ApplyServerSettings]: autobalance = %d; gri = %d", that->m_bShouldAutoBalance, ((ATrGameReplicationInfo*)that->GameReplicationInfo)->r_ServerConfig->bAutoBalanceInGame);
}

static bool shouldRemoveAllPlayersFromTeamsOnNextTick = false;

bool TrGameReplicationInfo_PostBeginPlay(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {
	Logger::debug("[PostBeginPlay]");
	{
		Utils::serverGameStatus = Utils::ServerGameStatus::PREROUND;
		std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);

		Utils::tr_gri = (ATrGameReplicationInfo*)dwCallingObject;
		Logger::debug("GRI set!");

		if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected()) {
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
				//Logger::debug("Removing player %s from team", Utils::f2std(pri->PlayerName).c_str());
				pri->Team->RemoveFromTeam((APlayerController*)pri->Owner);
			}
			else {
				//Logger::debug("PRI %s has no team or owner", Utils::f2std(pri->PlayerName).c_str());
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
	Logger::debug("Changing map to %s...", mapName.c_str());
	std::wstring wideMapName(mapName.begin(), mapName.end());
	wchar_t* mapNameData = new wchar_t[wideMapName.size() + 1];
	wcscpy(mapNameData, wideMapName.c_str());

	Utils::tr_gri->WorldInfo->eventServerTravel(FString(mapNameData), false, false);

	delete mapNameData;
}

void TAServer::Client::handler_Launcher2GameNextMapMessage(const json& msgBody) {
	if (g_config.serverMode != ServerMode::TASERVER || !g_TAServerClient.isConnected()) return;
	if (!Utils::tr_gri || !Utils::tr_gri->WorldInfo) return;

	if (g_config.serverSettings.mapRotation.empty()) {
		Utils::tr_gri->WorldInfo->eventServerTravel(FString(L"?restart"), false, false);
	}
	else {
		// TODO: Implement random map mode
		g_config.serverSettings.mapRotationIndex = (g_config.serverSettings.mapRotationIndex + 1) % g_config.serverSettings.mapRotation.size();
		std::string nextMapName = g_config.serverSettings.mapRotation[g_config.serverSettings.mapRotationIndex];
		performMapChange(nextMapName);
	}

}

static bool cachedWasInOvertime = false;
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
			Logger::debug("Overtime changed!");
			if (!cachedWasInOvertime) {
				Logger::debug("Sending overtime time update");
				updateMatchTime(true);
			}
			cachedWasInOvertime = Utils::tr_gri->WorldInfo->Game->bOverTime;
		}
	}
}