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

void TrGame_SendShowSummary(ATrGame* that, ATrGame_execSendShowSummary_Parms* params, void* result, Hooks::CallInfo callInfo) {
	// Seeing as we don't support the match end screen anyway, this can be blackholed
	// It causes the 'getting stuck in the menu glitch'
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

			}
			else {
			}
		}

		// Re-send game balance info to all players
		// This lets us mutate game balance between maps
		g_DCServer.forAllKnownConnections([](DCServer::Server* srv, std::shared_ptr<DCServer::PlayerConnection> pconn) {
			// Send the player the current balance state
			srv->sendGameBalanceDetailsMessage(pconn,
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

	// Reset team scores
	for (int i = 0; i < Utils::tr_gri->Teams.Count; ++i) {
		Utils::tr_gri->Teams.Data[i]->Score = 0;
	}

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

void TrPawn_TakeDamage(ATrPawn* that, ATrPawn_eventTakeDamage_Parms* params, void* result, Hooks::CallInfo callinfo) {
	if (!g_config.serverSettings.UseGOTYShieldPack) {
		// Use OOTB TakeDamage / shield pack behaviour
		that->eventTakeDamage(params->DamageAmount, params->EventInstigator, params->HitLocation, 
			params->Momentum, params->DamageType, params->HitInfo, params->DamageCauser);
		return;
	}

	AActor* DamagingActor = NULL;
	float ColRadius = 0.f, ColHeight = 0.f;
	float DamageScale = 0.f, Dist = 0.f, ScaledDamage = 0.f, EnergyDrainAmount = 0.f, DamageWithoutNewPlayerAssist = 0.f;
	FVector Dir;
	APawn* DamagingPawn = NULL;

	ATrGame* TrG = NULL;
	UClass* TrDamageType = NULL;
	ATrPlayerReplicationInfo* TrPRI = NULL, *DamagingPawnTrPRI = NULL;
	UTrValueModifier* VM = NULL, *DamagingPawnVM = NULL;
	ATrProjectile* TrProj = NULL;
	float DamageScaleWithoutNewPlayerAssist = 0.f;
	ATrPlayerController* TrEventInstigator = NULL;
	bool bIsEventInstigatorDeployableController = false;
	bool bIgnoreSecondaryEffectsOnSelf = false;
	bool bClearRage = false;

	TrDamageType = params->DamageType;

	TrPRI = that->GetTribesReplicationInfo();
	if (TrPRI) {
		VM = TrPRI->GetCurrentValueModifier();
		if (VM) {
			bIgnoreSecondaryEffectsOnSelf = VM->m_bIgnoreGrenadeSecondaryOnSelf;
		}
	}

	// Get direction and distance of the shot, and use it to calculate scaling / falloff
	DamageScale = 1.0;
	DamagingActor = ((UTrDmgType_Base*)UTrDmgType_Base::StaticClass()->Default)->GetActorCausingDamage(params->EventInstigator, params->DamageCauser);
	TrProj = (ATrProjectile*)DamagingActor;
	if (DamagingActor) {
		that->GetBoundingCylinder(&ColRadius, &ColHeight);
		Dir = that->Subtract_VectorVector(that->Location, DamagingActor->Location);
		if (TrProj) {
			if (TrProj->m_bIsBullet) {
				Dir = that->Subtract_VectorVector(TrProj->r_vSpawnLocation, params->HitLocation);
			}
			else {
				Dir = that->Subtract_VectorVector(that->Location, params->HitLocation);
			}
		}
		Dist = that->VSize(Dir);
		Dir = that->Normal(Dir);
		Dist = that->FMax(Dist - ColRadius, 0.f);

		DamageScale = that->GetDamageScale(params->DamageCauser, Dist, TrDamageType, params->EventInstigator, VM, &DamageScaleWithoutNewPlayerAssist);
	}

	TrG = (ATrGame*)that->WorldInfo->Game;

	if (DamageScale > 0) {
		ScaledDamage = DamageScale * params->DamageAmount;
		DamageWithoutNewPlayerAssist = DamageScaleWithoutNewPlayerAssist * params->DamageAmount;

		// Track statistics, accolades, awards
		if (that->Role == ROLE_Authority && params->EventInstigator) {
			that->LastHitInfo.Distance = 0;
			that->LastHitInfo.Type = params->DamageType;
			that->LastHitInfo.Causer = params->DamageCauser;
			that->LastHitInfo.Amount = ScaledDamage;

			// Recalculate distance from actual player if needed
			if (params->EventInstigator->Pawn && params->EventInstigator->Pawn != that) {
				Dir = that->Subtract_VectorVector(that->Location, params->EventInstigator->Pawn->Location);
				that->LastHitInfo.Distance = that->VSize(Dir);
			}
		}

		DamagingPawn = (APawn*)DamagingActor;
		if (!DamagingPawn && DamagingActor) {
			DamagingPawn = DamagingActor->Instigator;
		}

		// Drain energy
		if (TrDamageType) {
			UTrDmgType_Base* dmgTypeDef = (UTrDmgType_Base*)TrDamageType->Default;
			if (dmgTypeDef->m_EnergyDrainAmount > 0 && DamagingPawn && ((DamagingPawn == that && !bIgnoreSecondaryEffectsOnSelf) || DamagingActor->GetTeamNum() != that->GetTeamNum())) {
				EnergyDrainAmount = dmgTypeDef->m_EnergyDrainAmount;

				DamagingPawnTrPRI = (ATrPlayerReplicationInfo*)DamagingPawn->PlayerReplicationInfo;
				if (DamagingPawnTrPRI) {
					DamagingPawnVM = DamagingPawnTrPRI->GetCurrentValueModifier();
					if (DamagingPawnVM) {
						EnergyDrainAmount *= 1.0f + DamagingPawnVM->m_fEnergyDrainPctBuff;
					}
				}

				that->ConsumePowerPool(EnergyDrainAmount);
				that->SyncClientCurrentPowerPool();
			}
		}

		// Healing?
		if (ScaledDamage < 0) {
			that->DoRepairs(params->DamageAmount, params->EventInstigator, params->DamageCauser, params->DamageType);
			if (that->Stats) {
				// Disabled stats updating as it was causing a crash...
				//that->Stats->UpdateDamage((ATrPlayerController*)params->EventInstigator, ((UTrDmgType_Base*)that->LastHitInfo.Type->Default)->DBWeaponId, 
				//	ScaledDamage, that->LastHitInfo.Distance, false, false);
			}
			return;
		}

		// Only allow damaging team mate if friendly fire is on
		if (!that->WorldInfo->GRI->r_bFriendlyFire && DamagingActor && DamagingActor->GetTeamNum() == that->GetTeamNum() 
			&& (!DamagingPawn || DamagingPawn->Controller != that->Controller)) {
			return;
		}


		Logger::debug("params->EventInstigator = %p", params->EventInstigator);
		if (params->EventInstigator) {
			TrEventInstigator = (ATrPlayerController*)params->EventInstigator;

			if (!TrEventInstigator) {
				Logger::debug("Failed to cast EventInstigator");
				// Try getting an instigator from a turret
				if (params->EventInstigator->IsA(ATrDeployableController::StaticClass())) {
					TrEventInstigator = ((ATrDeployableController*)params->EventInstigator)->m_SpawnedFromController;
					bIsEventInstigatorDeployableController = true;
					Logger::debug("Got Deployable instigator");
				}
			}
			else if (that->Role == ROLE_Authority && params->EventInstigator != that->Controller && TrEventInstigator->GetTeamNum() == that->GetTeamNum()) {
				// Track the friendly fire the instigator has done, which may be used to kick them
				TrEventInstigator->FriendlyFireDamage += ScaledDamage;
				TrEventInstigator->CheckFriendlyFireDamage();
			}

			if (TrEventInstigator && TrEventInstigator != that->Controller) {
				// Play effects for hitting someone
				Logger::debug("Showing overhead number");
				TrEventInstigator->ServerShowOverheadNumber(DamageWithoutNewPlayerAssist, 
					((UTrDmgType_Base*)TrDamageType->Default)->ModifyOverheadNumberLocation(that->Location), that->Location.Z, that->bShieldAbsorb);

				if (!bIsEventInstigatorDeployableController) {
					Logger::debug("Doing FlashShooterHitReticule");
					TrEventInstigator->FlashShooterHitReticule(DamageWithoutNewPlayerAssist, false, that->GetTeamNum());
				}
			}
			else {
				Logger::debug("Not showing overhead number or doing FlashShooterHitReticule");
			}

			// Take shield pack damage from energy rather than health
			Logger::debug("Damage before GetUnshieldedDamage: %f", ScaledDamage);
			ScaledDamage = that->GetUnshieldedDamage(ScaledDamage);
			Logger::debug("Damage after GetUnshieldedDamage: %f", ScaledDamage);

			that->RememberLastDamager(params->EventInstigator, ScaledDamage, DamagingActor, params->DamageAmount);

			params->EventInstigator = that->CheckTribesTurretInstigator(params->EventInstigator, params->DamageCauser);

			// Potential Energy
			TrProj = (ATrProjectile*)DamagingActor;
			if (VM && 
				((VM->m_bPotentialEnergy && TrProj && !TrProj->m_bIsBullet) 
					|| (VM->m_bPotentialEnergyFallDamage && TrDamageType->Name == UTrDmgType_Fell::StaticClass()->Name))) {
				that->ConsumePowerPool(VM->m_fPotentialEnergyDamageTransferPct * ScaledDamage * -1);
				that->SyncClientCurrentPowerPool();
			}

			// Rage
			if (that->r_bIsRaged && params->EventInstigator->Pawn == that) {
				bClearRage = true;
				ScaledDamage = 0.f;
			}

			// Call superclass TakeDamage
			that->AUTPawn::eventTakeDamage(ScaledDamage, params->EventInstigator, params->HitLocation, 
				params->Momentum, params->DamageType, params->HitInfo, params->DamageCauser);

			// If the player has <= 0 health and isn't dead, they should die
			if (that->Role == ROLE_Authority && that->Health <= 0 && strcmp(that->GetStateName().GetName(), "Dying") != 0) {
				that->Died(DamagingPawn->Controller, params->DamageType, params->HitLocation);
			}

			// Update stats
			if (TrG && params->EventInstigator && that->Stats) {
				bool bDidDie = that->Health <= 0;
				// Disabled stats updating as it was causing a crash...
				//that->Stats->UpdateDamage((ATrPlayerController*)params->EventInstigator, 
				//	((UTrDmgType_Base*)that->LastHitInfo.Type->Default)->DBWeaponId, 
				//	ScaledDamage, that->LastHitInfo.Distance, bDidDie, false);
			}

			// Clear rage if required
			if (bClearRage) {
				// Find the rage clearing timer
				FName rageTimerName;
				for (int i = 0; i < that->Timers.Count; ++i) {
					if (strcmp(that->Timers.Data[i].FuncName.GetName(), "ClearRagePerk") == 0) {
						rageTimerName = that->Timers.Data[i].FuncName.GetName();
						break;
					}
				}
				// Clear rage effect and timer
				if (rageTimerName.Index != 0) {
					that->ClearTimer(rageTimerName, NULL);
				}
				that->ClearRagePerk();
			}
		}
	}
}