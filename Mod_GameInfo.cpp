#include "Mods.h"

//void TrGame_PreBeginPlay(ATrGame* that, ATrGame_execPreBeginPlay_Parms* params, bool* result, Hooks::CallInfo* callInfo) {
//	// Ensure the tr_game is set
//	Logger::debug("[TrGame.PreBeginPlay]");
//	//if (!Utils::tr_game || Utils::tr_game != that) {
//	//	Utils::tr_game = that;
//	//}
//	that->PreBeginPlay();
//}

//void TrGame_RequestTeam(ATrGame* that, ATrGame_execRequestTeam_Parms* params, bool* result, Hooks::CallInfo* callInfo) {
//	// Ensure the tr_game is set
//	//if (!Utils::tr_game || Utils::tr_game != that) {
//	//	Utils::tr_game = that;
//	//}
//
//	Logger::debug("[RequestTeam] requested team = %d", params->RequestedTeamNum);
//	*result = that->RequestTeam(params->RequestedTeamNum, params->C);
//	if (!*result) return;
//
//	if (g_config.serverMode == ServerMode::TASERVER && g_TAServerClient.isConnected()) {
//		// Send team info
//		ATrGameReplicationInfo* gri = (ATrGameReplicationInfo*)that->WorldInfo->Game->GameReplicationInfo;
//		if (!gri) return;
//
//		std::map<long long, int> playerToTeamId;
//		for (int i = 0; i < gri->PRIArray.Count; ++i) {
//			ATrPlayerReplicationInfo* pri = (ATrPlayerReplicationInfo*)gri->PRIArray.GetStd(i);
//			// There's always an extra 'player' with ID 0 connected; this is used by the real login servers to scrape info (we think)
//			// not useful here, ignore it
//			if (TAServer::netIdToLong(pri->UniqueId) == 0) continue;
//			playerToTeamId[TAServer::netIdToLong(pri->UniqueId)] = pri->GetTeamNum();
//		}
//
//		g_TAServerClient.sendTeamInfo(playerToTeamId);
//	}
//}



//void StartPollingForGameInfoChanges() {
//	Logger::debug("About to launch polling thread");
//	static std::thread g_GameInfoPollingThread([&] {
//		Logger::debug("In polling thread");
//		// Don't bother polling if sending the info is not needed
//		if (g_config.serverMode != ServerMode::TASERVER) return;
//
//		int beScoreCached = 0;
//		int dsScoreCached = 0;
//		Logger::debug("beginning polling");
//		//while (true) {
//		//	Logger::debug("In poll loop");
//		//	if (Utils::tr_game && g_TAServerClient.isConnected()) {
//		//		Logger::debug("Polling...");
//		//		int beScoreCurrent = Utils::tr_game->Teams[0]->Score;
//		//		int dsScoreCurrent = Utils::tr_game->Teams[1]->Score;
//		//		if (beScoreCurrent != beScoreCached || dsScoreCurrent != dsScoreCached) {
//		//			Logger::debug("Update found! New scores are (%d, %d)", beScoreCurrent, dsScoreCurrent);
//		//			g_TAServerClient.sendScoreInfo(beScoreCurrent, dsScoreCurrent);
//		//			beScoreCached = beScoreCurrent;
//		//			dsScoreCached = dsScoreCurrent;
//		//		}
//		//	}
//		//	 //Poll for score changes once every two seconds
//		//	std::this_thread::sleep_for(std::chrono::seconds(2));
//		//}
//	});	
//	g_GameInfoPollingThread.detach();
//}