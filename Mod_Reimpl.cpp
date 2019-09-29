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
// Requires reimplementation of kill assists since we can't easily manipulate TArrays that get dynamically allocated
////////////////////////

static std::vector<FAssistInfo> getKillAssisters(ATrPawn* that) {
    if (!that || !that->PlayerReplicationInfo) return std::vector<FAssistInfo>();

    long long playerId = Utils::netIdToLong(that->PlayerReplicationInfo->UniqueId);

    return TenantedDataStore::playerData.get(playerId).assistInfo;
}

static void addKillAssister(ATrPawn* that, FAssistInfo curAssister) {
    if (!that || !that->PlayerReplicationInfo) return;

    long long playerId = Utils::netIdToLong(that->PlayerReplicationInfo->UniqueId);

    TenantedDataStore::PlayerSpecificData pData = TenantedDataStore::playerData.get(playerId);

    pData.assistInfo.push_back(curAssister);

    TenantedDataStore::playerData.set(playerId, pData);
}

static void updateKillAssister(ATrPawn* that, ATrPlayerController* damager, FAssistInfo newAssisterInfo) {
    if (!that || !that->PlayerReplicationInfo) return;

    long long playerId = Utils::netIdToLong(that->PlayerReplicationInfo->UniqueId);

    TenantedDataStore::PlayerSpecificData pData = TenantedDataStore::playerData.get(playerId);

    for (int i = 0; i < pData.assistInfo.size(); ++i) {
        if (pData.assistInfo[i].m_Damager != newAssisterInfo.m_Damager) continue;

        pData.assistInfo[i] = newAssisterInfo;
        TenantedDataStore::playerData.set(playerId, pData);
        return;
    }
}

static void clearKillAssisters(ATrPawn* that) {
    if (!that || !that->PlayerReplicationInfo) return;

    long long playerId = Utils::netIdToLong(that->PlayerReplicationInfo->UniqueId);

    TenantedDataStore::PlayerSpecificData pData = TenantedDataStore::playerData.get(playerId);

    pData.assistInfo.clear();

    TenantedDataStore::playerData.set(playerId, pData);
}

void TrPawn_TakeDamage(ATrPawn* that, ATrPawn_eventTakeDamage_Parms* params, void* result, Hooks::CallInfo* callinfo) {
    EventLogger::TakeDamageEvent ev;
    ev.damage_prescale_amount = params->DamageAmount;
    if (!g_config.serverSettings.UseGOTYShieldPack) {
        // Use OOTB TakeDamage / shield pack behaviour
        that->eventTakeDamage(params->DamageAmount, params->EventInstigator, params->HitLocation,
            params->Momentum, params->DamageType, params->HitInfo, params->DamageCauser);
        ev.collection_return_point = 0;
        EventLogger::log(ev);
        return;
    }

    AActor* DamagingActor = ((UTrDmgType_Base*)UTrDmgType_Base::StaticClass()->Default)->GetActorCausingDamage(params->EventInstigator, params->DamageCauser);

    if (DamagingActor && DamagingActor->Class && !strcmp(DamagingActor->Class->GetName(), "TrProj_Honorfusor")) {
        // Use OOTB TakeDamage, because honorfusor is borked in the reimplementation
        that->eventTakeDamage(params->DamageAmount, params->EventInstigator, params->HitLocation,
            params->Momentum, params->DamageType, params->HitInfo, params->DamageCauser);
        ev.collection_return_point = 1;
        EventLogger::log(ev);
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
        ev.victim_name = Utils::f2std(TrPRI->PlayerName);

        VM = TrPRI->GetCurrentValueModifier();
        if (VM) {
            bIgnoreSecondaryEffectsOnSelf = VM->m_bIgnoreGrenadeSecondaryOnSelf;
        }
    }

    // Get direction and distance of the shot, and use it to calculate scaling / falloff
    DamageScale = 1.0;
    if (DamagingActor) {
        ev.damaging_actor = DamagingActor->GetStringName();
        that->GetBoundingCylinder(&ColRadius, &ColHeight);
        Dir = Geom::sub(that->Location, DamagingActor->Location);
        ev.damaging_actor_dir_type = 0;
        if (DamagingActor->IsA(ATrProjectile::StaticClass())) {
            TrProj = (ATrProjectile*)DamagingActor;
            if (TrProj->m_bIsBullet) {
                Dir = Geom::sub(TrProj->r_vSpawnLocation, params->HitLocation);
                ev.damaging_actor_dir_type = 1;
            }
            else {
                Dir = Geom::sub(that->Location, params->HitLocation);
                ev.damaging_actor_dir_type = 2;
            }
        }

        Dist = Geom::vSize(Dir);
        Dir = Geom::normal(Dir);
        Dist = that->FMax(Dist - ColRadius, 0.f);
        ev.damage_distance = Dist;
        DamageScale = that->GetDamageScale(params->DamageCauser, Dist, TrDamageType, params->EventInstigator, VM, &DamageScaleWithoutNewPlayerAssist);
        ev.damage_scale = DamageScale;
    }

    TrG = (ATrGame*)that->WorldInfo->Game;

    if (DamageScale <= 0) {
        ev.collection_return_point = 2;
        EventLogger::log(ev);
        return;
    }
    
    ScaledDamage = DamageScale * params->DamageAmount;
    ev.damage_preshield_amount = ScaledDamage;
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

    if (DamagingActor) {
        if (DamagingActor->IsA(APawn::StaticClass())) {
            DamagingPawn = (APawn*)DamagingActor;
        }
        else {
            DamagingPawn = DamagingActor->Instigator;
        }
    }    

    // Drain energy
    if (TrDamageType) {
        ev.damage_type = TrDamageType->GetStringName();
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

            ev.energy_drained = EnergyDrainAmount;

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
            //    ScaledDamage, that->LastHitInfo.Distance, false, false);
        }
        ev.collection_return_point = 3;
        EventLogger::log(ev);
        return;
    }

    // Only allow damaging team mate if friendly fire is on
    if (!that->WorldInfo->GRI->r_bFriendlyFire && DamagingActor && DamagingActor->GetTeamNum() == that->GetTeamNum()
        && (!DamagingPawn || DamagingPawn->Controller != that->Controller)) {
        ev.collection_return_point = 4;
        EventLogger::log(ev);
        return;
    }

    if (!params->EventInstigator) {
        ev.collection_return_point = 5;
        EventLogger::log(ev);
        return;
    }

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

    if (TrEventInstigator && TrEventInstigator->IsA(ATrPlayerController::StaticClass()) && TrEventInstigator->PlayerReplicationInfo) {
        ev.damaging_player_name = Utils::f2std(TrEventInstigator->PlayerReplicationInfo->PlayerName);
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
    ATrPawn_execGetUnshieldedDamage_Parms getUnshieldedParams = { ScaledDamage, 0.f };
    TrPawn_GetUnshieldedDamage(that, &getUnshieldedParams, &ScaledDamage);
    ev.damage_unshielded_amount = ScaledDamage;

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
        ev.was_rage_impulse = true;
        bClearRage = true;
        ScaledDamage = 0.f;
    }

    // Call superclass TakeDamage
    that->AUTPawn::eventTakeDamage(ScaledDamage, params->EventInstigator, params->HitLocation,
        params->Momentum, params->DamageType, params->HitInfo, params->DamageCauser);

    // If the player has <= 0 health and isn't dead, they should die
    if (that->Role == ROLE_Authority && that->Health <= 0 && strcmp(that->GetStateName().GetName(), "Dying") != 0) {
        ev.caused_death = true;
        that->Died(DamagingPawn->Controller, params->DamageType, params->HitLocation);
    }

    // Update stats
    if (TrG && params->EventInstigator && that->Stats) {
        bool bDidDie = that->Health <= 0;
        // Disabled stats updating as it was causing a crash...
        //that->Stats->UpdateDamage((ATrPlayerController*)params->EventInstigator, 
        //    ((UTrDmgType_Base*)that->LastHitInfo.Type->Default)->DBWeaponId, 
        //    ScaledDamage, that->LastHitInfo.Distance, bDidDie, false);
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

    ev.collection_return_point = 6;
    EventLogger::log(ev);
}

void TrPawn_GetUnshieldedDamage(ATrPawn* that, ATrPawn_execGetUnshieldedDamage_Parms* params, float* result) {
    if (!g_config.serverSettings.UseGOTYShieldPack) {
        *result = that->GetUnshieldedDamage(params->inputDamage);
        return;
    }

    float shieldAbsorb = that->m_fShieldMultiple;

    ATrPlayerReplicationInfo* pri = that->GetTribesReplicationInfo();
    if (pri) {
        UTrValueModifier* vm = pri->GetCurrentValueModifier();
        if (vm) {
            shieldAbsorb -= vm->m_fShieldPackEffectivenessBuff;
        }
    }

    if (!that->r_bIsShielded) {
        *result = params->inputDamage;
        return;
    }

    if (that->m_fCurrentPowerPool >= params->inputDamage * shieldAbsorb) {
        that->ConsumePowerPool(params->inputDamage * shieldAbsorb);
        that->SyncClientCurrentPowerPool();
        *result = 0;
    }
    else {
        float tmp = params->inputDamage - (that->m_fCurrentPowerPool / shieldAbsorb);
        that->ConsumePowerPool(that->m_fCurrentPowerPool);
        that->SyncClientCurrentPowerPool();
        *result = tmp;
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
    
    std::vector<FAssistInfo> assisters = getKillAssisters(that);

    for (FAssistInfo assistInfo : assisters) {
        if (!assistInfo.m_Damager || assistInfo.m_Damager != TrDamager) continue;

        if (assistInfo.m_fDamagerTime < (that->WorldInfo->TimeSeconds - that->m_fSecondsBeforeAutoHeal)) {
            // Reset damage done since player has healed since being shot
            assistInfo.m_AccumulatedDamage = params->DamageAmount;
        }
        else {
            assistInfo.m_AccumulatedDamage += params->DamageAmount;
        }

        assistInfo.m_fDamagerTime = that->WorldInfo->TimeSeconds;

        updateKillAssister(that, assistInfo.m_Damager, assistInfo);
        return;
    }

    // Add new record since we didn't have one previously
    if (params->Damager) {
        addKillAssister(that, that->CreateAssistRecord(params->Damager, params->DamageAmount));
    }
}

bool TrPawn_RechargeHealthPool(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {
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
    clearKillAssisters(that);

    that->ClientPlayHealthRegenEffect();

    return true;
}

void TrPawn_GetLastDamager(ATrPawn* that, ATrPawn_execGetLastDamager_Parms* params, ATrPlayerController** result) {
    if (!g_config.serverSettings.UseGOTYShieldPack) {
        *result = that->GetLastDamager();
        return;
    }

    int BestIndex = CONST_INDEX_NONE;
    float MostRecentTimestamp = 0;

    std::vector<FAssistInfo> killAssisters = getKillAssisters(that);

    for (int i = 0; i < killAssisters.size(); ++i) {
        if (killAssisters[i].m_fDamagerTime > MostRecentTimestamp) {
            MostRecentTimestamp = killAssisters[i].m_fDamagerTime;
            BestIndex = i;
        }
    }

    *result = BestIndex == CONST_INDEX_NONE ? NULL : killAssisters[BestIndex].m_Damager;
}

void TrPawn_ProcessKillAssists(ATrPawn* that, ATrPawn_execProcessKillAssists_Parms* params) {
    if (!g_config.serverSettings.UseGOTYShieldPack) {
        that->ProcessKillAssists(params->Killer);
        return;
    }

    ATrPlayerController* TrKiller = (ATrPlayerController*)params->Killer;

    std::vector<FAssistInfo> killAssisters = getKillAssisters(that);

    for (FAssistInfo assister : killAssisters) {
        if (!assister.m_Damager || !assister.m_Damager->PlayerReplicationInfo) continue;

        if (assister.m_Damager != TrKiller &&
                assister.m_AccumulatedDamage > that->HealthMax * that->m_AssistDamagePercentQualifier &&
                assister.m_fDamagerTime > (that->WorldInfo->TimeSeconds - that->m_fSecondsBeforeAutoHeal)) {
            //if (that->Stats) that->Stats->AddAssist(assister.m_Damager);
            ATrPlayerReplicationInfo* DamagerPRI = (ATrPlayerReplicationInfo*)assister.m_Damager->PlayerReplicationInfo;
            DamagerPRI->m_nAssists++;
            assister.m_Damager->m_AccoladeManager->GiveAssist();
        }
    }

    clearKillAssisters(that);

    std::vector<FAssistInfo> postVals = getKillAssisters(that);
}

void TrPawn_SetShieldPackActive(ATrPawn* that, ATrPawn_execSetShieldPackActive_Parms* params) {
    if (!g_config.serverSettings.UseGOTYShieldPack) {
        that->SetShieldPackActive(params->bActive, params->ShieldEnergyAbsorb);
        return;
    }

    if (that->Role == ROLE_Authority) {
        that->r_bIsShielded = params->bActive;
        that->PlayShieldPackEffect();
    }
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

void TrPlayerController_PlayerWalking_ProcessMove(ATrPlayerController* that, APlayerController_execProcessMove_Parms* params) {
    ATrPawn* TrP = (ATrPawn*)that->Pawn;
    if (!TrP) return;

    if (that->m_bBlink) {
        

        if (TrP->Physics != PHYS_Falling && TrP->Physics != PHYS_Flying) {
            TrP->SetPhysics(PHYS_Falling);
        }

        ATrPlayerController_execGetBlinkPackAccel_Parms callParams;
        callParams.BlinkPackPctEffectiveness = 0;
        callParams.newAccel = FVector(0, 0, 0);
        TrPlayerController_GetBlinkPackAccel(that, &callParams, NULL, NULL);
        TrP->Velocity = Geom::add(TrP->Velocity, callParams.newAccel);

        if (that->Role == ROLE_Authority) {
            TrP->r_nBlinked++;
        }

        TrP->PlayBlinkPackEffect();

        if (that->Role == ROLE_Authority) {
            that->Pawn->SetRemoteViewPitch(that->Rotation.Pitch);
        }
        return;
    }

    if (that->m_bPressingJetpack) {
        if (that->Pawn->Physics != PHYS_Flying) {
            if (that->m_bJumpJet) {
                that->bPressedJump = true;
                that->CheckJumpOrDuck();
            }

            that->Pawn->SetPhysics(PHYS_Flying);
        }
    }
    else {
        if (that->Pawn->Physics == PHYS_Flying) {
            that->Pawn->SetPhysics(PHYS_Falling);
        }
    }

    if (that->Role == ROLE_Authority) {
        that->Pawn->SetRemoteViewPitch(that->Rotation.Pitch);
    }

    that->Pawn->Acceleration = params->newAccel;

    if (that->Pawn->Physics != PHYS_Flying) {
        that->CheckJumpOrDuck();
    }
}

void TrPlayerController_GetBlinkPackAccel(ATrPlayerController* that, ATrPlayerController_execGetBlinkPackAccel_Parms* params, void* result, Hooks::CallInfo* callInfo) {
    if (g_config.serverSettings.RageThrustPackDependsOnCapperSpeed) {
        that->GetBlinkPackAccel(&(params->newAccel), &(params->BlinkPackPctEffectiveness));
        return;
    }

    FVector NewAccel;
    float BlinkPackPctEffectiveness;

    ATrPawn* TrP = (ATrPawn*)that->Pawn;
    ATrPlayerReplicationInfo* TrPRI = (ATrPlayerReplicationInfo*)TrP->PlayerReplicationInfo;
    ATrDevice_Blink* BlinkPack = (ATrDevice_Blink*)that->GetDeviceByEquipPoint(EQP_Pack);

    if (!TrP || !BlinkPack) {
        return;
    }

    if (!TrP->IsA(ATrPawn::StaticClass()) || !BlinkPack->IsA(ATrDevice_Blink::StaticClass())) return;

    FVector ViewPos;
    FRotator ViewRot;
    that->eventGetPlayerViewPoint(&ViewPos, &ViewRot);
    
    // Start with a local-space impulse amount
    //NewAccel = BlinkPack->GetBlinkImpulse();
    NewAccel = BlinkPack->m_vBlinkImpulse;
    if (TrPRI) {
        UTrValueModifier* VM = TrPRI->GetCurrentValueModifier();
        if (VM) {
            NewAccel = Geom::mult(NewAccel, 1.0 + VM->m_fBlinkPackPotencyBuffPct);
        }
    }

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
    if ((Geom::dot(Geom::normal(TrP->Velocity), Geom::rotationToVector(ViewRot)) >= 0) && PawnSpeed > BlinkPack->m_fSpeedCapThresholdStart) {
        BlinkPackSpeedCapMultiplier = that->Lerp(1.0f, BlinkPack->m_fSpeedCapPct, that->FPctByRange(that->Min(PawnSpeed, BlinkPack->m_fSpeedCapThreshold), BlinkPack->m_fSpeedCapThresholdStart, BlinkPack->m_fSpeedCapThreshold));
    }
    BlinkPackPctEffectiveness *= BlinkPackSpeedCapMultiplier;

    // Apply the effectiveness debuf
    NewAccel = Geom::mult(NewAccel, BlinkPackPctEffectiveness);

    // Take energy from the player
    //BlinkPack->OnBlink(BlinkPackPctEffectiveness, false);
    float VMMultiplier = 1.0f;
    if (TrPRI) {
        UTrValueModifier* VM = TrPRI->GetCurrentValueModifier();
        if (VM) {
            VMMultiplier = 1.0f - VM->m_fPackEnergyCostBuffPct;
        }
    }
    TrP->ConsumePowerPool(BlinkPack->m_fPowerPoolCost * VMMultiplier);
    TrP->SyncClientCurrentPowerPool();

    // Out params
    params->newAccel = NewAccel;
    params->BlinkPackPctEffectiveness = BlinkPackPctEffectiveness;
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

static long long getPlayerIdFromRemoteArxBuster(ATrDevice_RemoteArxBuster* that) {
    if (!that || !that->Owner) return -1;

    APawn* pawn = (APawn*)that->Owner;
    if (!pawn->PlayerReplicationInfo) return -1;
    return Utils::netIdToLong(pawn->PlayerReplicationInfo->UniqueId);
}

static void addJackalRound(ATrDevice_RemoteArxBuster* that, ATrProj_RemoteArxBuster* proj) {
    long long playerId = getPlayerIdFromRemoteArxBuster(that);
    if (playerId == -1) return;

    TenantedDataStore::PlayerSpecificData pData = TenantedDataStore::playerData.get(playerId);

    pData.remoteArxRounds.push_back(proj);

    TenantedDataStore::playerData.set(playerId, pData);
}

static void removeJackalRounds(ATrDevice_RemoteArxBuster* that, int idx, int count) {
    long long playerId = getPlayerIdFromRemoteArxBuster(that);
    if (playerId == -1) return;

    TenantedDataStore::PlayerSpecificData pData = TenantedDataStore::playerData.get(playerId);

    pData.remoteArxRounds.erase(pData.remoteArxRounds.begin() + idx, pData.remoteArxRounds.begin() + idx + count);

    TenantedDataStore::playerData.set(playerId, pData);
}

static std::vector<ATrProj_RemoteArxBuster*> getJackalRounds(ATrDevice_RemoteArxBuster* that) {
    long long playerId = getPlayerIdFromRemoteArxBuster(that);
    if (playerId == -1) return std::vector<ATrProj_RemoteArxBuster*>();

    return TenantedDataStore::playerData.get(playerId).remoteArxRounds;
}


void ATrDevice_RemoteArxBuster_PerformInactiveReload(ATrDevice_RemoteArxBuster* that, ATrDevice_RemoteArxBuster_execPerformInactiveReload_Parms* params) {
    if (!g_config.serverSettings.UseGOTYJackalAirburst) {
        that->PerformInactiveReload();
        return;
    }

    if (getJackalRounds(that).empty()) {
        that->ATrDevice::PerformInactiveReload();
    }
}

void ATrDevice_RemoteArxBuster_ProjectileFire(ATrDevice_RemoteArxBuster* that, ATrDevice_RemoteArxBuster_execProjectileFire_Parms* params, AProjectile** result) {
    if (!g_config.serverSettings.UseGOTYJackalAirburst) {
        *result = that->ProjectileFire();
        return;
    }

    AProjectile* ProjCreated = that->ATrDevice::ProjectileFire();

    // Add the newly created projectile to the rounds ready to fire
    ATrProj_RemoteArxBuster* TrProj = (ATrProj_RemoteArxBuster*)ProjCreated;
    if (TrProj) {
        addJackalRound(that, TrProj);
    }

    that->SetLeftArmVisible(true);

    *result = ProjCreated;
}

void ATrDevice_RemoteArxBuster_ActivateRemoteRounds(ATrDevice_RemoteArxBuster* that, ATrDevice_RemoteArxBuster_execActivateRemoteRounds_Parms* params) {
    if (!g_config.serverSettings.UseGOTYJackalAirburst) {
        that->ActivateRemoteRounds(params->bDoNoDamage);
        return;
    }

    // Explode all rounds on reload
    std::vector< ATrProj_RemoteArxBuster*> rounds = getJackalRounds(that);
    for (ATrProj_RemoteArxBuster* proj : rounds) {
        if (params->bDoNoDamage) {
            proj->Damage = 0;
        }
        proj->m_bIsDetonating = true; 
        proj->Explode(proj->Location, FVector(0, 0, 1));
    }

    // Clear the remote rounds list
    removeJackalRounds(that, 0, rounds.size());

    if (that->Role == ROLE_Authority) {
        that->SetTimer(0.75, false, "HideArmTimer", NULL);
    }
}

////// Fix for pilot ejection perk not working: in OOTB grav always ejects and others don't

static bool shouldPlayerEjectOnDeath(ATrVehicle* that, ATrPlayerReplicationInfo* pri) {
    // Check for ejection seat perk
    UTrValueModifier* vm = pri->GetCurrentValueModifier();
    if (vm && vm->m_bEjectionSeat) {
        return true;
    }

    // Check if vehicle should always eject
    switch (that->m_VehicleType) {
    case VEHICLE_GravCycle:
        return g_config.serverSettings.GravCycleEjectionSeat;
    case VEHICLE_Beowulf:
        return g_config.serverSettings.BeowulfEjectionSeat;
    case VEHICLE_Shrike:
        return g_config.serverSettings.ShrikeEjectionSeat;
    case VEHICLE_Havoc:
        return g_config.serverSettings.HavocEjectionSeat;
    case VEHICLE_HERC:
        return g_config.serverSettings.HERCEjectionSeat;
    default:
        return false;
    }
}

void ATrVehicle_Died(ATrVehicle* that, ATrVehicle_execDied_Parms* params, bool* result) {
    for (int i = 0; i < that->Seats.Count; ++i) {
        APawn* storagePawn = that->Seats.GetStd(i).StoragePawn;        if (!storagePawn || !storagePawn->IsA(ATrPawn::StaticClass())) continue;

        ATrPawn* trPawn = (ATrPawn*)that->Seats.GetStd(i).StoragePawn;

        ATrPlayerReplicationInfo* pri = (ATrPlayerReplicationInfo*)trPawn->GetTribesReplicationInfo();
        if (!pri) continue;


        if (shouldPlayerEjectOnDeath(that, pri)) {
            trPawn->GoInvulnerable(0.01f);
            that->EjectSeat(i);
            that->RidingPawnLeave(i, true);
        }
    }

    // Award the killer a vehicle destruction
    ATrPlayerController* killerPC = (ATrPlayerController*)params->Killer;
    if (killerPC && killerPC->IsA(ATrPlayerController::StaticClass()) && killerPC->m_AccoladeManager && that->GetTeamNum() == killerPC->GetTeamNum()) {
        // Stats disabled due to crashes
        //if (that->Stats) that->Stats->AddVehicleDestruction(killerPC);
        killerPC->m_AccoladeManager->VehicleDestroyed(that);
    }

    // Apply damage to passengers and eject them
    for (int i = 1; i < that->Seats.Count; ++i) {
        APawn* storagePawn = that->Seats.GetStd(i).StoragePawn;
        if (!storagePawn || !storagePawn->IsA(ATrPawn::StaticClass())) continue;

        ATrPawn* trPawn = (ATrPawn*)that->Seats.GetStd(i).StoragePawn;
        //if (!trPawn || that->Seats.GetStd(i).SeatPawn) continue;
        that->RidingPawnLeave(i, true);

        AController* instigator = params->Killer;
        if (params->Killer == that->Controller) {
            instigator = trPawn->Controller;
        }
        trPawn->eventTakeDamage(trPawn->HealthMax / 2, instigator, that->LastTakeHitInfo.HitLocation, that->LastTakeHitInfo.Momentum, params->DamageType, FTraceHitInfo(), that);
    }

    // Report back to vehicle station that spawned this, to update its inventory
    if (that->m_OwnerStation) {
        // Reimplmented AddVehicleToPackedList... it wasn't working for reasons unknown
        int curNum = that->m_OwnerStation->GetNumVehiclesSpawnedByType(that->m_VehicleType);
        int newPackedSection = ((curNum - 1) & CONST_SPAWNED_VEHICLE_LIST_MASK) << (4 * that->m_VehicleType);
        int zeroedPackedSection = ~(CONST_SPAWNED_VEHICLE_LIST_MASK << (4 * that->m_VehicleType)) & that->m_OwnerStation->r_nSpawnedVehicles;
        that->m_OwnerStation->r_nSpawnedVehicles = zeroedPackedSection | newPackedSection;

        //that->m_OwnerStation->AddVehicleToPackedList(that->m_VehicleType, -1);
    }

    *result = that->AUTVehicle::Died(params->Killer, params->DamageType, params->HitLocation);
}

////////////////////////
// Vehicles cost credits
////////////////////////

void TrVehicleStation_AbleToSpawnVehicleType(ATrVehicleStation* that, ATrVehicleStation_execAbleToSpawnVehicleType_Parms* params, bool* result) {
    if (g_config.serverSettings.VehiclesEarnedWithCredits) {
        *result = that->GetNumVehiclesSpawnedByType(params->VehicleType) < that->GetMaxVehicleCountAllowed(params->VehicleType) && that->m_VehiclePad;
    }
    else {
        *result = that->AbleToSpawnVehicleType(params->VehicleType);
    }
}

int getVehicleCost(ATrPlayerController* that, EVehicleTypes vehicleType) {
    int cost = 0;

    ATrGameReplicationInfo* gri = (ATrGameReplicationInfo*)that->WorldInfo->GRI;
    if (gri && gri->r_ServerConfig) {
        cost = gri->r_ServerConfig->VehiclePrices[vehicleType];
    }

    // Vehicle cost perk
    ATrPlayerReplicationInfo* pri = (ATrPlayerReplicationInfo*)that->PlayerReplicationInfo;
    if (pri) {
        UTrValueModifier* vm = pri->GetCurrentValueModifier();
        if (vm) {
            cost *= that->FClamp(1.0 - vm->m_fVehicleCostBuffPct, 0.0, 1.0);
        }
    }

    return cost;
}

bool TrPlayerController_ServerRequestSpawnVehicle(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {
    if (!g_config.serverSettings.VehiclesEarnedWithCredits) return false;

    ATrPlayerController* that = (ATrPlayerController*)dwCallingObject;
    ATrPlayerController_execServerRequestSpawnVehicle_Parms* params = (ATrPlayerController_execServerRequestSpawnVehicle_Parms*)pParams;

    EVehicleTypes vehicleType = (EVehicleTypes)((ATrVehicle*)params->VehicleClass->Default)->m_VehicleType;

    int creditCost = getVehicleCost(that, vehicleType);
    if (!that->InTraining() && creditCost > that->GetCurrentCredits()) {
        // Cannot afford vehicle
        return true;
    }

    ATrVehicleStation* vehicleStation = (ATrVehicleStation*)that->m_CurrentStation;
    if (vehicleStation && vehicleStation->IsA(ATrVehicleStation::StaticClass())) {
        if (vehicleStation->RequestSpawnVehicle((unsigned char)vehicleType)) {
            if (!that->InTraining()) {
                // Charge credits for the purchase
                that->ModifyCredits(-creditCost, false);
            }
        }
    }

    return true;
}