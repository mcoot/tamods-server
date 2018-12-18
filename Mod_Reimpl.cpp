#include "Mods.h"

////////////////////////
// TakeDamage reimplementation
// Used to modify shield pack behaviour
////////////////////////

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

		if (params->EventInstigator) {
			TrEventInstigator = (ATrPlayerController*)params->EventInstigator;

			if (!TrEventInstigator) {
				// Try getting an instigator from a turret
				if (params->EventInstigator->IsA(ATrDeployableController::StaticClass())) {
					TrEventInstigator = ((ATrDeployableController*)params->EventInstigator)->m_SpawnedFromController;
					bIsEventInstigatorDeployableController = true;
				}
			}
			else if (that->Role == ROLE_Authority && params->EventInstigator != that->Controller && TrEventInstigator->GetTeamNum() == that->GetTeamNum()) {
				// Track the friendly fire the instigator has done, which may be used to kick them
				TrEventInstigator->FriendlyFireDamage += ScaledDamage;
				TrEventInstigator->CheckFriendlyFireDamage();
			}

			if (TrEventInstigator && TrEventInstigator != that->Controller) {
				// Play effects for hitting someone
				TrEventInstigator->ServerShowOverheadNumber(DamageWithoutNewPlayerAssist,
					((UTrDmgType_Base*)TrDamageType->Default)->ModifyOverheadNumberLocation(that->Location), that->Location.Z, that->bShieldAbsorb);

				if (!bIsEventInstigatorDeployableController) {
					TrEventInstigator->FlashShooterHitReticule(DamageWithoutNewPlayerAssist, false, that->GetTeamNum());
				}
			}

			// Take shield pack damage from energy rather than health
			ScaledDamage = that->GetUnshieldedDamage(ScaledDamage);

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

////////////////////////
// Laser Targeter modifications
// Used to re-enable call-ins
////////////////////////

// Cache of players with laser targeter instances currently firing and when they started
// So we can have a buildup and cooldown time
struct _LaserTargetCacheInfo {
	long long playerId = -1;
	bool isFiring = false;
	float lastTimeFiringStarted = 0;
	float lastTimeInvStationCalled = 0;
};

static std::map<long long, _LaserTargetCacheInfo> firingLaserTargetTimes;

static _LaserTargetCacheInfo ltCache_get(ATrDevice_LaserTargeter* that) {
	if (!that->Owner || !that->Owner->IsA(APawn::StaticClass())) return _LaserTargetCacheInfo();

	APawn* pawn = (APawn*)that->Owner;
	if (!pawn->PlayerReplicationInfo) return _LaserTargetCacheInfo();
	long long playerId = TAServer::netIdToLong(pawn->PlayerReplicationInfo->UniqueId);

	_LaserTargetCacheInfo v;
	if (firingLaserTargetTimes.find(playerId) != firingLaserTargetTimes.end()) {
		v = firingLaserTargetTimes[playerId];
	}
	v.playerId = playerId;

	return v;
}

static void ltCache_setFiring(ATrDevice_LaserTargeter* that) {
	if (!that->WorldInfo || !that->Owner || !that->Owner->IsA(APawn::StaticClass())) return;

	_LaserTargetCacheInfo v = ltCache_get(that);
	v.lastTimeFiringStarted = that->WorldInfo->TimeSeconds;
	v.isFiring = true;
	
	firingLaserTargetTimes[v.playerId] = v;
}

static void ltCache_recordCallIn(ATrDevice_LaserTargeter* that) {
	if (!that->WorldInfo || !that->Owner || !that->Owner->IsA(APawn::StaticClass())) return;

	_LaserTargetCacheInfo v = ltCache_get(that);
	v.lastTimeInvStationCalled = that->WorldInfo->TimeSeconds;
	Logger::debug("Recorded inv call-in!");
	firingLaserTargetTimes[v.playerId] = v;
}

static void ltCache_stopFiring(ATrDevice_LaserTargeter* that) {
	if (!that->Owner || !that->Owner->IsA(APawn::StaticClass())) return;

	_LaserTargetCacheInfo v = ltCache_get(that);
	v.isFiring = false;

	firingLaserTargetTimes[v.playerId] = v;
}

static bool ltCache_isFiring(ATrDevice_LaserTargeter* that) {
	if (!that->WorldInfo || !that->Owner || !that->Owner->IsA(APawn::StaticClass())) return false;
	_LaserTargetCacheInfo v = ltCache_get(that);

	return v.isFiring;
}

static float ltCache_buildUpTimePassed(ATrDevice_LaserTargeter* that) {
	if (!that->WorldInfo || !that->Owner || !that->Owner->IsA(APawn::StaticClass())) return false;
	_LaserTargetCacheInfo v = ltCache_get(that);

	return that->WorldInfo->TimeSeconds - v.lastTimeFiringStarted;
}

static float ltCache_cooldownTimePassed(ATrDevice_LaserTargeter* that) {
	if (!that->WorldInfo || !that->Owner || !that->Owner->IsA(APawn::StaticClass())) return false;
	_LaserTargetCacheInfo v = ltCache_get(that);

	return that->WorldInfo->TimeSeconds - v.lastTimeInvStationCalled;
}

static bool ltCache_canFire(ATrDevice_LaserTargeter* that, float buildUpSeconds, float cooldownSeconds) {
	if (!that->WorldInfo || !that->Owner || !that->Owner->IsA(APawn::StaticClass())) return false;

	bool doneBuildUp = ltCache_buildUpTimePassed(that) >= buildUpSeconds;
	bool doneCooldown = ltCache_cooldownTimePassed(that) >= cooldownSeconds;

	Logger::debug("Checked canFire: %d, %d (%f vs %f), %d (%f vs %f),", ltCache_isFiring(that), doneBuildUp, ltCache_buildUpTimePassed(that), buildUpSeconds, doneCooldown, ltCache_cooldownTimePassed(that), cooldownSeconds);

	return ltCache_isFiring(that) && doneBuildUp && doneCooldown;
}

void ResetLaserTargetCallInCache() {
	firingLaserTargetTimes.clear();
}

bool getTargetLocationAndNormal(ATrDevice_LaserTargeter* that, FVector& startLoc, FVector& targetLoc, FVector& targetLocNormal) {
	FVector startTrace = that->InstantFireStartTrace();
	FVector endTrace = that->InstantFireEndTrace(startTrace);

	targetLocNormal = FVector(0, 0, 1);

	TArray<FImpactInfo> impactList;

	FImpactInfo testImpact = that->CalcWeaponFire(startTrace, endTrace, FVector(0, 0, 0), 0, &impactList);

	if (that->NotEqual_VectorVector(testImpact.HitLocation, startTrace) || that->NotEqual_VectorVector(testImpact.HitLocation, endTrace)) {
		targetLoc = testImpact.HitLocation;
		targetLocNormal = testImpact.HitNormal;
	}

	startLoc = startTrace;

	// TODO: Check validity of target location!!!
	return testImpact.HitActor != NULL;
}

void TrDevice_LaserTargeter_OnStartConstantFire(ATrDevice_LaserTargeter* that, ATrDevice_LaserTargeter_execOnStartConstantFire_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	Logger::debug("OnStartConstantFire!");
	ltCache_setFiring(that);
	that->OnStartConstantFire();
}

static float TrDevice_LaserTargeter_CalcHUDAimChargePercent(ATrDevice_LaserTargeter* that) {
	Logger::debug("Calculating aim charge percent for targeter!");
	// Don't show call-in progress if not firing or on cooldown
	if (ltCache_cooldownTimePassed(that) < g_config.serverSettings.InventoryCallInCooldownTime) {
		return 0.f;
	}

	float progress = ltCache_buildUpTimePassed(that) / g_config.serverSettings.InventoryCallInBuildUpTime;
	Logger::debug("Calculated aim charge percent for targeter: %f!", progress);
	return that->FClamp(progress, 0.f, 1.f);
}

void TrDevice_CalcHUDAimChargePercent(ATrDevice* that, ATrDevice_execCalcHUDAimChargePercent_Parms* params, float* result, Hooks::CallInfo callInfo) {
	Logger::debug("Calculating aim charge percent!");
	*result = that->CalcHUDAimChargePercent();

	if (that->IsA(ATrDevice_LaserTargeter::StaticClass())) {
		*result = TrDevice_LaserTargeter_CalcHUDAimChargePercent((ATrDevice_LaserTargeter*)that);
	}
}

void TrDevice_LaserTargeter_OnEndConstantFire(ATrDevice_LaserTargeter* that, ATrDevice_LaserTargeter_execOnEndConstantFire_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	if (!g_config.serverSettings.EnableInventoryCallIn) {
		Logger::debug("Not doing custom OnEndConstantFire");
		// Call-In logic not required
		that->OnEndConstantFire();
		ltCache_stopFiring(that);
		return;
	}

	bool canFireCallIn = ltCache_canFire(that, g_config.serverSettings.InventoryCallInBuildUpTime, g_config.serverSettings.InventoryCallInCooldownTime);
	ltCache_stopFiring(that);

	if (!canFireCallIn) {
		Logger::debug("Not firing call-in in OnEndConstantFire");
		that->OnEndConstantFire();
		return;
	}

	// Call in an inventory station
	ltCache_recordCallIn(that);

	static ATrCallIn_SupportInventory* callIn = (ATrCallIn_SupportInventory*)that->Spawn(ATrCallIn_SupportInventory::StaticClass(), that->Owner, FName(), FVector(), FRotator(), NULL, false);
	callIn->Initialize(0, 0, 0);

	Logger::debug("Initialised call-in");

	FVector laserStart;
	FVector laserEnd;
	FVector hitNormal;
	bool hasLaserHit = getTargetLocationAndNormal(that, laserStart, laserEnd, hitNormal);
	that->UpdateTarget(hasLaserHit, laserEnd);

	Logger::debug("Attempting callin at coords (%f, %f, %f)!", laserEnd.X, laserEnd.Y, laserEnd.Z);

	if (callIn->FireCompletedCallIn(1, laserEnd, hitNormal)) {
		Logger::debug("Successfully fired callin");
	}
	else {
		Logger::debug("Failed to fire callin");
	}

	that->OnEndConstantFire();
}