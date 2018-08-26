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


static void updateMatchTime() {
	if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected()) {
		int timeLeft = Utils::tr_gri->TimeLimit != 0 ? Utils::tr_gri->RemainingTime : Utils::tr_gri->ElapsedTime;
		g_TAServerClient.sendMatchTime(timeLeft, Utils::tr_gri->bWarmupRound);
	}
}

void TrGame_RequestTeam(ATrGame* that, ATrGame_execRequestTeam_Parms* params, bool* result, Hooks::CallInfo* callInfo) {
	Logger::debug("[RequestTeam] requested team = %d", params->RequestedTeamNum);
	*result = that->RequestTeam(params->RequestedTeamNum, params->C);
	if (!*result) return;

	if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected()) {
		// Send team info
		ATrGameReplicationInfo* gri = (ATrGameReplicationInfo*)that->WorldInfo->Game->GameReplicationInfo;
		if (!gri) return;

		std::map<long long, int> playerToTeamId;
		for (int i = 0; i < gri->PRIArray.Count; ++i) {
			ATrPlayerReplicationInfo* pri = (ATrPlayerReplicationInfo*)gri->PRIArray.GetStd(i);
			// There's always an extra 'player' with ID 0 connected; this is used by the real login servers to scrape info (we think)
			// not useful here, ignore it
			if (TAServer::netIdToLong(pri->UniqueId) == 0) continue;
			playerToTeamId[TAServer::netIdToLong(pri->UniqueId)] = pri->GetTeamNum();
		}

		g_TAServerClient.sendTeamInfo(playerToTeamId);
	}
}

void UTGame_EndGame(AUTGame* that, AUTGame_execEndGame_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	Logger::debug("[UTGame.EndGame] called");
	that->EndGame(params->Winner, params->Reason);

	// Tell TAServer that the game is ending
	if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected()) {
		//updateMatchTime(false);
		g_TAServerClient.sendMatchEnded();
	}
}

void UTGame_ProcessServerTravel(AUTGame* that, AUTGame_execProcessServerTravel_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	// Invalidate the global GRI
	{
		// Ensure latest data has been sent to the login server
		pollForGameInfoChanges();
		// Invalidate the GRI
		std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
		Logger::debug("Invalidating GRI");
		Utils::tr_gri = NULL;
	}
	Logger::debug("About to process server travel");
	that->ProcessServerTravel(params->URL, params->bAbsolute);
}

static bool changeMapOnNextTick = false;

bool TrGameReplicationInfo_Tick(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {
	if (!Utils::tr_gri) {
		{
			std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
			Utils::tr_gri = (ATrGameReplicationInfo*)dwCallingObject;
			Logger::debug("GRI set!");
		}
		if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected() && Utils::tr_gri) {
			// GRI should only be set at the start of the match -> ergo not counting down
			//updateMatchTime(false);
		}
	}
	if (changeMapOnNextTick) {
		changeMapOnNextTick = false;
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
	changeMapOnNextTick = true;
}

static void performMapChange(std::string mapName) {
	Logger::debug("Changing map to %s...", mapName.c_str());
	std::wstring wideMapName(mapName.begin(), mapName.end());
	wchar_t* mapNameData = new wchar_t[wideMapName.size() + 1];
	wcscpy(mapNameData, wideMapName.c_str());
	Logger::debug("About to trigger travel...", mapName.c_str());

	Utils::tr_gri->WorldInfo->eventServerTravel(FString(mapNameData), false, false);

	Logger::debug("Travel triggered...", mapName.c_str());
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

void pollForGameInfoChanges() {

	// Lock the GRI to ensure it can't be unset during polling
	std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
	if (!Utils::tr_gri) {
		return;
	}
	
	checkForScoreChange();
	//updateMatchTime();
}