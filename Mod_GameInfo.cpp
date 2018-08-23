#include "Mods.h"

void TrGame_RequestTeam(ATrGame* that, ATrGame_execRequestTeam_Parms* params, bool* result, Hooks::CallInfo* callInfo) {
	// Set the global GRI reference on team join (to catch case where injection happens after the map starts)
	if (!Utils::tr_gri && that->WorldInfo->GRI && that->WorldInfo->GRI->IsA(ATrGameReplicationInfo::StaticClass())) {
		{
			std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
			Utils::tr_gri = (ATrGameReplicationInfo*)that->WorldInfo->GRI;
			Logger::debug("GRI set!");
		}
		
	}
	

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

void TrGame_EndGame(ATrGame* that, ATrGame_execEndGame_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	// This may not actually be the end of the game, as EndGame is also called when overtime starts...
	// So check
	bool actuallyEnding = true;
	if (!that->CheckEndGame(params->Winner, params->Reason) && !that->bForceEndGame) {
		actuallyEnding = false;
	}

	// Even if the game isn't ending, call the original function (it sets the overtime timer etc. in this case)
	that->EndGame(params->Winner, params->Reason);
	if (!actuallyEnding) return;

	//// Invalidate the global GRI
	//{
	//	std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
	//	Logger::debug("Invalidating GRI");
	//	Utils::tr_gri = NULL;
	//}

	// Tell TAServer that the game is ending
	if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected()) {
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

	that->ProcessServerTravel(params->URL, params->bAbsolute);
}

void TrGameReplicationInfo_PostBeginPlay(ATrGameReplicationInfo* that, ATrGameReplicationInfo_eventPostBeginPlay_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	Logger::debug("[PostBeginPlay] called");
	that->PostBeginPlay();

	
}

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

void pollForGameInfoChanges() {
	// Lock the GRI to ensure it can't be unset during polling
	std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
	if (!Utils::tr_gri) {
		return;
	}
	
	checkForScoreChange();
	updateMatchTime();
}