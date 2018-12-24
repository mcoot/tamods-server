#include "Mods.h"

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
// Inv Stations restore energy
////////////////////////

void TrStation_PawnEnteredStation(ATrStation* that, ATrStation_execPawnEnteredStation_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	
	that->bForceNetUpdate = true;

	if (!that->r_CurrentPawn) {
		
	}

	that->PawnEnteredStation(params->P);
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