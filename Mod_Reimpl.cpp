#include "Mods.h"

////////////////////////
// Nova Colt Ping Dependency
////////////////////////

void TrDevice_NovaSlug_FireAmmunition(ATrDevice_NovaSlug* that, ATrDevice_NovaSlug_execFireAmmunition_Parms* params, void* result, Hooks::CallInfo callInfo) {
	//that->r_bReadyToFire = true;
	TrDevice_FireAmmunition(that, NULL, NULL, Hooks::CallInfo());
}

void TrDevice_FireAmmunition(ATrDevice* that, ATrDevice_execFireAmmunition_Parms* params, void* result, Hooks::CallInfo callInfo) {
	if (!that->IsA(ATrDevice_NovaSlug::StaticClass())) {
		that->ATrDevice::FireAmmunition();
		return;
	}
	if (!that->ReplicateAmmoOnWeaponFire() && !that->IsA(ATrDevice_NovaSlug::StaticClass())) {
		that->r_bReadyToFire = false;
	}

	if (!that->m_bAllowHoldDownFire) {
		that->m_bWantsToFire = false;
	}

	that->AUTWeapon::FireAmmunition();
	if (!that->m_bAllowHoldDownFire) {
		that->StopFire(0);
	}

	that->PlayFireAnimation(0);
	that->eventCauseMuzzleFlash();
	that->ShakeView();

	that->PayAccuracyForShot();
	that->eventUpdateShotsFired(false);

	bool bKickedBack = that->AddKickback();

	ATrPawn* pawn = (ATrPawn*)that->Instigator;

	if (pawn) {
		ATrPlayerController* pc = (ATrPlayerController*)pawn->Controller;

		if (bKickedBack && pc && pawn->Weapon == that) {
			FVector StartTrace = pawn->GetWeaponStartTraceLocation(that);
			FVector AimVector = Geom::rotationToVector(that->GetAimForCamera(StartTrace));
			FVector EndTrace = that->Add_VectorVector(StartTrace, that->Multiply_VectorFloat(AimVector, that->GetWeaponRange()));
			FVector kbVec = that->Normal(that->Subtract_VectorVector(EndTrace, pawn->GetPawnViewLocation()));
			pc->OnKickback(that->Subtract_RotatorRotator(Geom::vectorToRotation(kbVec), pawn->eventGetViewRotation()), that->m_fKickbackBlendOutTime);
		}
	}
}

////////////////////////
// TakeDamage reimplementation
// Used to modify shield pack behaviour
////////////////////////

void TrPawn_TakeDamage(ATrPawn* that, ATrPawn_eventTakeDamage_Parms* params, void* result, Hooks::CallInfo* callinfo) {
	if (!g_config.serverSettings.UseGOTYShieldPack) {
		// Use OOTB TakeDamage / shield pack behaviour
		that->eventTakeDamage(params->DamageAmount, params->EventInstigator, params->HitLocation,
			params->Momentum, params->DamageType, params->HitInfo, params->DamageCauser);
		return;
	}

	AActor* DamagingActor = ((UTrDmgType_Base*)UTrDmgType_Base::StaticClass()->Default)->GetActorCausingDamage(params->EventInstigator, params->DamageCauser);

	if (!strcmp(DamagingActor->Class->GetName(), "TrProj_Honorfusor")) {
		// Use OOTB TakeDamage, because honorfusor is borked in the reimplementation
		that->eventTakeDamage(params->DamageAmount, params->EventInstigator, params->HitLocation,
			params->Momentum, params->DamageType, params->HitInfo, params->DamageCauser);
		return;
	}


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
	if (DamagingActor) {
		that->GetBoundingCylinder(&ColRadius, &ColHeight);
		Dir = that->Subtract_VectorVector(that->Location, DamagingActor->Location);
		if (DamagingActor->IsA(ATrProjectile::StaticClass())) {
			TrProj = (ATrProjectile*)DamagingActor;
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
			if (params->EventInstigator->IsA(ATrDeployableController::StaticClass())) {
				TrEventInstigator = ((ATrDeployableController*)params->EventInstigator)->m_SpawnedFromController;
				bIsEventInstigatorDeployableController = true;
			}
			else {
				TrEventInstigator = (ATrPlayerController*)params->EventInstigator;

				if (that->Role == ROLE_Authority && params->EventInstigator != that->Controller && TrEventInstigator->GetTeamNum() == that->GetTeamNum()) {
					// Track the friendly fire the instigator has done, which may be used to kick them
					TrEventInstigator->FriendlyFireDamage += ScaledDamage;
					TrEventInstigator->CheckFriendlyFireDamage();
				}
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

			ATrPawn_execRememberLastDamager_Parms rememberLastDamagerParams;
			rememberLastDamagerParams.Damager = params->EventInstigator;
			rememberLastDamagerParams.DamageAmount = ScaledDamage;
			rememberLastDamagerParams.DamagingActor = DamagingActor;
			rememberLastDamagerParams.UnscaledDamageAmount = params->DamageAmount;
			TrPawn_RememberLastDamager(that, &rememberLastDamagerParams);

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

void TrPawn_RememberLastDamager(ATrPawn* that, ATrPawn_execRememberLastDamager_Parms* params) {
	if (!g_config.serverSettings.UseGOTYShieldPack) {
		that->RememberLastDamager(params->Damager, params->DamageAmount, params->DamagingActor, params->UnscaledDamageAmount);
		return;
	}

	ATrPlayerController* OwnerController;
	if (that->Owner && that->Owner->IsA(ATrVehicle::StaticClass())) {
		OwnerController = (ATrPlayerController*)(((ATrVehicle*)that->Owner)->Controller);
	}
	else {
		OwnerController = (ATrPlayerController*)that->Owner;
	}

	// Kill cam info
	if (OwnerController && OwnerController->IsA(ATrPlayerController::StaticClass()) && params->Damager && params->Damager->Pawn) {
		ATrPawn* TrP = (ATrPawn*)params->Damager->Pawn;
		if (TrP && TrP->IsA(ATrPawn::StaticClass())) {
			ATrTurretPawn* TrTP = (ATrTurretPawn*)TrP;

			// Check if the damager was a turret/deployable, or a player
			if (TrTP && TrTP->IsA(ATrTurretPawn::StaticClass()) && TrTP->m_OwnerDeployable) {
				OwnerController->ClientSetLastDamagerInfo(100 * ((float)TrTP->m_OwnerDeployable->r_Health)/((float)TrTP->m_OwnerDeployable->r_MaxHealth), TrTP->m_OwnerDeployable->r_nUpgradeLevel);
			}
			else {
				OwnerController->ClientSetLastDamagerInfo(TrP->GetHealthPct(), 0);
			}
		}
	}

	// Remember the actual actor that damaged us
	if (OwnerController && OwnerController->IsA(ATrPlayerController::StaticClass())) {
		OwnerController->m_LastDamagedBy = params->DamagingActor;
	}

	// If we damaged ourselves, always apply a timestamp
	if (params->Damager && params->Damager == OwnerController) {
		if (params->DamageAmount > 0) {
			that->m_fLastDamagerTimeStamp = that->WorldInfo->TimeSeconds;
		}
		return;
	}

	// Ignore timestamp for people on the same team unless friendly fire is on
	bool bSameTeam = OwnerController && OwnerController->IsA(ATrPlayerController::StaticClass()) && params->Damager->GetTeamNum() == OwnerController->GetTeamNum();
	if (bSameTeam && !that->WorldInfo->GRI->r_bFriendlyFire) return;

	ATrPlayerController* TrDamager = (ATrPlayerController*)params->Damager;

	// Apply timestamp only if actual damage was done
	if (params->DamageAmount > 0) {
		that->m_fLastDamagerTimeStamp = that->WorldInfo->TimeSeconds;
	}

	// Only give score and assist status for damage on opposing team
	if (!bSameTeam) {
		if (TrDamager && TrDamager->IsA(ATrPlayerController::StaticClass())) TrDamager->CashForDamage(params->DamageAmount);
	}

	for (int i = 0; i < that->m_KillAssisters.Count; ++i) {
		FAssistInfo* assistInfo = that->m_KillAssisters.Get(i);
		if (assistInfo->m_Damager != TrDamager) continue;

		if (assistInfo->m_fDamagerTime < (that->WorldInfo->TimeSeconds - that->m_fSecondsBeforeAutoHeal)) {
			// Reset damage done since player has healed since being shot
			assistInfo->m_AccumulatedDamage = params->DamageAmount;
		}
		else {
			assistInfo->m_AccumulatedDamage += params->DamageAmount;
		}

		assistInfo->m_fDamagerTime = that->WorldInfo->TimeSeconds;
		return;
	}

	// Disabled as this causes a crash when you start regening after someone shot you
	// Because we're manipulating TAMods managed memory and then the UScript VM goes and tries to remove from the array later...
	// Add new assist record since we didn't have one already
	//bool foundEmptySpot = false;
	//for (int i = 0; i < that->m_KillAssisters.Count; ++i) {
	//	if (!that->m_KillAssisters.GetStd(i).m_Damager) {
	//		foundEmptySpot = true;
	//		that->m_KillAssisters.Set(i, that->CreateAssistRecord(params->Damager, params->DamageAmount));
	//		break;
	//	}
	//}
	//if (!foundEmptySpot) {
	//	that->m_KillAssisters.Add(that->CreateAssistRecord(params->Damager, params->DamageAmount));
	//}
	//if (that->m_KillAssisters.Count < 32) {
	//	for (int i = that->m_KillAssisters.Count; i < 32; ++i) {
	//		FAssistInfo emptyInfo;
	//		emptyInfo.m_Damager = NULL;
	//		emptyInfo.m_fDamagerTime = 0;
	//		emptyInfo.m_AccumulatedDamage = 0;
	//		that->m_KillAssisters.Add(emptyInfo);
	//	}
	//}
	//for (int i = 0;; ++i) {
	//	if (!that->m_KillAssisters.GetStd(i).m_Damager) {
	//		that->m_KillAssisters.Set(i, that->CreateAssistRecord(params->Damager, params->DamageAmount));
	//		break;
	//	}
	//}
}

bool TrPawn_RechargeHealthPool(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {
	// This hook is disabled as I can't fix assist
	return false;

	ATrPawn* that = (ATrPawn*)dwCallingObject;
	ATrPawn_eventRechargeHealthPool_Parms* params = (ATrPawn_eventRechargeHealthPool_Parms*)pParams;

	if (!g_config.serverSettings.UseGOTYShieldPack) {
		return false;
	}

	if (that->Role != ROLE_Authority) return true;

	if (!that->GetCurrCharClassInfo()) return true;

	if (that->Health >= that->HealthMax) {
		if (that->r_bIsHealthRecharging) {
			ATrPlayerController* TrPC = (ATrPlayerController*)that->Owner;
			if (TrPC && TrPC->IsA(ATrPlayerController::StaticClass())) {
				if (TrPC->AlienFX && TrPC->EnableAlienFX) {
					TrPC->AlienFX->SetHealth(100);
				}

				if (TrPC->Stats) {
					// Disable stats due to crashes
					//TrPC->Stats->RegeneratedToFull(TrPC);
				}
			}
			that->r_bIsHealthRecharging = false;
			that->bNetDirty = true;
		}

		return true;
	}

	// Do not regen health if you are a flag carrier
	ATrPlayerReplicationInfo* TrPRI = that->GetTribesReplicationInfo();
	if (TrPRI && TrPRI->bHasFlag) {
		that->r_bIsHealthRecharging = false;
		that->bNetDirty = true;
		return true;
	}

	if (that->WorldInfo->TimeSeconds - that->m_fLastDamagerTimeStamp <= that->m_fSecondsBeforeAutoHeal) {
		that->r_bIsHealthRecharging = false;
		that->bNetDirty = true;
		return true;
	}
	
	float RechargeRateMultiplier = 1.0f;

	// Get skill / perk influence
	if (TrPRI) {
		UTrValueModifier* VM = TrPRI->GetCurrentValueModifier();
		if (VM) {
			RechargeRateMultiplier += VM->m_fHealthRegenRateBuffPct;
		}
	}

	float fRecharge = that->Health + ((UTrFamilyInfo*)that->GetCurrCharClassInfo()->Default)->m_fHealthPoolRechargeRate * RechargeRateMultiplier * params->DeltaSeconds * that->HealthMax;

	that->m_fTickedRegenDecimal += fRecharge - (float)that->Health;

	fRecharge += that->m_fTickedRegenDecimal;

	if (fRecharge > that->HealthMax) {
		fRecharge = that->HealthMax;
		that->m_fTickedRegenDecimal = 0;
	}
	else {
		that->m_fTickedRegenDecimal -= that->Abs((int)that->m_fTickedRegenDecimal);
	}

	that->Health = fRecharge;

	that->r_bIsHealthRecharging = true;
	that->bNetDirty = true;

	// Clear out any assists when regen begins
	for (int i = 0; i < that->m_KillAssisters.Count; ++i) {
		FAssistInfo emptyInfo;
		emptyInfo.m_Damager = NULL;
		emptyInfo.m_fDamagerTime = 0;
		emptyInfo.m_AccumulatedDamage = 0;
		that->m_KillAssisters.Set(i, emptyInfo);
	}

	that->ClientPlayHealthRegenEffect();

	return true;
}

////////////////////////
// Honorfusor hitreg fix
////////////////////////

void TrProj_Honorfusor_ProjectileHurtRadius(ATrProj_Honorfusor* that, ATrProj_Honorfusor_execProjectileHurtRadius_Parms* params, bool* result) {
	*result = that->ATrProjectile::ProjectileHurtRadius(params->HurtOrigin, params->HitNormal);
}

////////////////////////
// Thrust pack rage dependency fix
////////////////////////

void TrPlayerController_GetBlinkPackAccel(ATrPlayerController* that, ATrPlayerController_execGetBlinkPackAccel_Parms* params) {
	Logger::debug("GetBlinkPackAccel!");
	if (g_config.serverSettings.RageThrustPackDependsOnCapperSpeed) {
		that->GetBlinkPackAccel(&(params->newAccel), &(params->BlinkPackPctEffectiveness));
		return;
	}

	FVector NewAccel;
	float BlinkPackPctEffectiveness;

	ATrPawn* TrP = (ATrPawn*)that->Pawn;
	ATrDevice_Blink* BlinkPack = (ATrDevice_Blink*)that->GetDeviceByEquipPoint(EQP_Pack);

	if (!TrP->IsA(ATrPawn::StaticClass()) || !BlinkPack->IsA(ATrDevice_Blink::StaticClass())) return;

	FVector ViewPos;
	FRotator ViewRot;
	that->eventGetPlayerViewPoint(&ViewPos, &ViewRot);
	
	// Start with a local-space impulse amount
	NewAccel = BlinkPack->GetBlinkImpulse();

	// Transform from local to world space
	NewAccel = that->GreaterGreater_VectorRotator(NewAccel, ViewRot);

	// Always make sure we have upward impulse
	if (NewAccel.Z <= BlinkPack->m_fMinZImpulse) {
		NewAccel.Z = BlinkPack->m_fMinZImpulse;
	}

	// Modify acceleration based on the power pool
	BlinkPackPctEffectiveness = BlinkPack->m_fPowerPoolCost > 0.0f ? that->FClamp(TrP->GetPowerPoolPercent() * 100 / BlinkPack->m_fPowerPoolCost, 0.0f, 1.0f) : 1.0f ;

	// Modify the acceleration based on a speed cap
	float PawnSpeed = that->VSize(TrP->Velocity);
	float BlinkPackSpeedCapMultiplier = 1.0f;
	if ((that->Dot_VectorVector(that->Normal(TrP->Velocity), Geom::rotationToVector(ViewRot)) >= 0) && PawnSpeed > BlinkPack->m_fSpeedCapThreshold) {
		BlinkPackSpeedCapMultiplier = that->Lerp(1.0f, BlinkPack->m_fSpeedCapPct, that->FPctByRange(that->Min(PawnSpeed, BlinkPack->m_fSpeedCapThreshold), BlinkPack->m_fSpeedCapThresholdStart, BlinkPack->m_fSpeedCapThreshold));
	}
	BlinkPackPctEffectiveness *= BlinkPackSpeedCapMultiplier;

	// Apply the effectiveness debuf
	NewAccel = that->Multiply_FloatVector(BlinkPackPctEffectiveness, NewAccel);

	BlinkPack->OnBlink(BlinkPackPctEffectiveness, false);

	// Out params
	params->newAccel = NewAccel;
	params->BlinkPackPctEffectiveness = BlinkPackPctEffectiveness;
}

void TrDevice_Blink_OnBlink(ATrDevice_Blink* that, ATrDevice_Blink_execOnBlink_Parms* params) {
	that->OnBlink(params->PercentEffectiveness, params->wasRage && g_config.serverSettings.RageThrustPackDependsOnCapperSpeed);
}

////////////////////////
// Inv Stations restore energy
////////////////////////

void TrPawn_RefreshInventory(ATrPawn* that, ATrPawn_execRefreshInventory_Parms* params) {
	if (!params->bIsRespawn && g_config.serverSettings.InventoryStationsRestoreEnergy) {
		that->ConsumePowerPool(-1.0 * that->r_fMaxPowerPool);
	}
	that->RefreshInventory(params->bIsRespawn, params->bCallin);	
}

////////////////////////
// Sniper rifle fixes
////////////////////////

float TrDevice_SniperRifle_CalcHUDAimChargePercent(ATrDevice_SniperRifle* that) {
	if (that->WorldInfo->NetMode != NM_DedicatedServer && that->r_fAimChargeTime > 0) {
		return that->m_fAimChargeDeltaTime / that->r_fAimChargeTime;
	}
	return 0.0f;
}

void TrDevice_CalcHUDAimChargePercent(ATrDevice* that, ATrDevice_execCalcHUDAimChargePercent_Parms* params, float* result, Hooks::CallInfo* callInfo) {
	*result = that->CalcHUDAimChargePercent();

	if (that->IsA(ATrDevice_LaserTargeter::StaticClass())) {
		*result = TrDevice_LaserTargeter_CalcHUDAimChargePercent((ATrDevice_LaserTargeter*)that);
	}
	else if (that->IsA(ATrDevice_SniperRifle::StaticClass())) {
		*result = TrDevice_SniperRifle_CalcHUDAimChargePercent((ATrDevice_SniperRifle*)that);
	}
}

bool TrDevice_SniperRifle_Tick(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {
	ATrDevice_SniperRifle* that = (ATrDevice_SniperRifle*)dwCallingObject;
	ATrDevice_SniperRifle_eventTick_Parms* params = (ATrDevice_SniperRifle_eventTick_Parms*)pParams;

	if (!g_config.serverSettings.UseGOTYBXTCharging) {
		that->eventTick(params->DeltaTime);
		return true;
	}

	if (!that->Owner) {
		that->ATrDevice::eventTick(params->DeltaTime);
		return true;
	}
	ATrPawn* owner = (ATrPawn*)that->Owner;
	
	if (!that->Instigator || !that->Instigator->Controller) {
		that->ATrDevice::eventTick(params->DeltaTime);
		return true;
	}
	ATrPlayerController* pc = (ATrPlayerController*)that->Instigator->Controller;

	// If not zoomed, no charge
	if (pc->m_ZoomState == ZST_NotZoomed) {
		that->m_fAimChargeDeltaTime = 0.f;
	}
	else {
		float interpDeltaTime = params->DeltaTime;

		if (owner->Physics == PHYS_Falling || owner->Physics == PHYS_Flying || that->VSizeSq(owner->Velocity) > 490000) {
			interpDeltaTime *= 0.5f;
		}

		// Increment charge time
		that->m_fAimChargeDeltaTime += interpDeltaTime;
	}

	if (that->m_fAimChargeDeltaTime >= that->r_fAimChargeTime) {
		that->StopScopeRechargeSound();
	}

	that->ATrDevice::eventTick(params->DeltaTime);
	
	return true;
}

void ATrPlayerController_ResetZoomDuration(ATrPlayerController* that, ATrPlayerController_execResetZoomDuration_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	if (!g_config.serverSettings.UseGOTYBXTCharging) {
		that->ResetZoomDuration(params->bPlayRechargeSoundOnWeapon);
		return;
	}
	
	if (!that->Pawn) return;

	if (that->WorldInfo->NetMode == NM_DedicatedServer) return;

	that->c_fHUDZoomDuration = 0;

	if (!that->Pawn->Weapon->IsA(ATrDevice_SniperRifle::StaticClass())) return;

	ATrDevice_SniperRifle* dev = (ATrDevice_SniperRifle*)that->Pawn->Weapon;

	if (params->bPlayRechargeSoundOnWeapon && that->m_ZoomState != ZST_NotZoomed) {
		dev->PlayScopeRechargeSound();
	}
	else {
		dev->StopScopeRechargeSound();
	}
}

void TrDevice_SniperRifle_ModifyInstantHitDamage(ATrDevice_SniperRifle* that, ATrDevice_SniperRifle_execModifyInstantHitDamage_Parms* params, float* result, Hooks::CallInfo* callInfo) {
	if (!g_config.serverSettings.UseGOTYBXTCharging) {
		that->ModifyInstantHitDamage(params->FiringMode, params->Impact, params->Damage);
		return;
	}
	
	float damage = params->Damage;
	
	if (that->Instigator && that->Instigator->Controller) {
		ATrPlayerController* pc = (ATrPlayerController*)that->Instigator->Controller;

		if (pc && pc->m_ZoomState != ZST_NotZoomed) {
			// Polynomial scale, 16x^2
			float aimChargePct = that->FMin(that->r_fAimChargeTime, that->m_fAimChargeDeltaTime);
			aimChargePct = that->FMin( (that->m_fMultCoeff * aimChargePct * aimChargePct) / that->m_fDivCoeff, 1.0f);

			if (aimChargePct > 0 && that->r_fAimChargeTime > 0) {
				if (that->m_fMaxAimedDamage > damage) {
					damage += (that->m_fMaxAimedDamage - damage) * aimChargePct;
				}
			}
		}
	}

	// Reset the aim charge
	that->m_fAimChargeDeltaTime = 0.f;

	*result = that->ATrDevice::ModifyInstantHitDamage(params->FiringMode, params->Impact, damage);
}

////////////////////////
// Jackal Airburst reimplementation
////////////////////////

void ATrDevice_RemoteArxBuster_ActivateRemoteRounds(ATrDevice_RemoteArxBuster* that, ATrDevice_RemoteArxBuster_execActivateRemoteRounds_Parms* params) {
	if (!g_config.serverSettings.UseGOTYJackalAirburst) {
		that->ActivateRemoteRounds(params->bDoNoDamage);
		return;
	}

	Logger::debug("BOOMY BOOM BOOM");

	for (int i = 0; i < that->RemoteArxRounds.Count; ++i) {
		ATrProj_RemoteArxBuster* proj = that->RemoteArxRounds.GetStd(i);
		if (params->bDoNoDamage) {
			proj->Damage = 0;
		}
		proj->m_bIsDetonating = true;
		proj->Explode(proj->Location, FVector(0, 0, 1));
	}

	that->ActivateRemoteRounds(false);
}