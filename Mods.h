#pragma once

#include <sstream>
#include <list>
#include <random>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <boost/format.hpp>

#include "FunctionalUtils.h"

#include "TenantedDataStore.h"
#include "TAServerClient.h"
#include "DirectConnectionServer.h"

#include "Config.h"
#include "Utils.h"
#include "Data.h"
#include "Geom.h"
#include "Logger.h"
#include "LuaAPI.h"
#include "EventLogger.h"
//#include "TAServerClient.h"

// Hook called on server settings load
void TrServerSettingsInfo_LoadServerSettings(ATrServerSettingsInfo* that, ATrServerSettingsInfo_eventLoadServerSettings_Parms* params, void* result, Hooks::CallInfo* callInfo);

// Hook called to process the swapping to a new class
//void TrPlayerReplicationInfo_SwapToPendingCharClass(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execSwapToPendingCharClass_Parms* params, void* result, Hooks::CallInfo* callInfo);
// HookedVerifyCharacter always returns true
void TrPlayerReplicationInfo_VerifyCharacter(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execVerifyCharacter_Parms* params, bool* result);
// Hook called when the server tries to retrieve a player's loadout
void TrPlayerReplicationInfo_GetCharacterEquip(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execGetCharacterEquip_Parms* params, void* result, Hooks::CallInfo* callInfo);
// Hook called when the server tries to fill in gaps in a player's loadout with defaults
void TrPlayerReplicationInfo_ResolveDefaultEquip(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execResolveDefaultEquip_Parms* params, void* result, Hooks::CallInfo* callInfo);
void TrPlayerController_GetFamilyInfoFromId(ATrPlayerController* that, ATrPlayerController_execGetFamilyInfoFromId_Parms* params, UClass** result);

// Hook called on GRI tick
bool TrGameReplicationInfo_PostBeginPlay(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult);
bool TrGameReplicationInfo_Tick(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult);

void TrGame_ApplyServerSettings(ATrGame* that, ATrGame_execApplyServerSettings_Parms* params, void* result, Hooks::CallInfo* callInfo);

// Fix for the 'getting stuck in the menus after map change' glitch
void TrGame_SendShowSummary(ATrGame* that, ATrGame_execSendShowSummary_Parms* params, void* result, Hooks::CallInfo callInfo);

// Hook called when a player wants to switch teams (or to spectator)
void TrGame_RequestTeam(ATrGame* that, ATrGame_execRequestTeam_Parms* params, bool* result, Hooks::CallInfo* callInfo);

// Hook called on game/round start
bool UTGame_MatchInProgress_BeginState(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult);

// Hook called on game end (or when game enters overtime!)
void UTGame_EndGame(AUTGame* that, AUTGame_execEndGame_Parms* params, void* result, Hooks::CallInfo* callInfo);

// Hook called during map transition
void UTGame_ProcessServerTravel(AUTGame* that, AUTGame_execProcessServerTravel_Parms* params, void* result, Hooks::CallInfo* callInfo);

void TrDevice_GetReloadTime(ATrDevice* that, ATrDevice_execGetReloadTime_Parms* params, float* result, Hooks::CallInfo* callInfo);

// Hook called when the server attempts to update ping using the ingame mechanism (blackholed)
void PlayerController_ServerUpdatePing(APlayerController* that, APlayerController_execServerUpdatePing_Parms* params, void* result, Hooks::CallInfo* callInfo);

// Hook called when the playercontroller calculates how long the respawn time should be
void TrPlayerController_GetRespawnDelayTotalTime(ATrPlayerController* that, ATrPlayerController_execGetRespawnDelayTotalTime_Parms* params, float* result, Hooks::CallInfo* callInfo);

// Fix helping Nova Colt ping dependency
void TrDevice_FireAmmunition(ATrDevice* that, ATrDevice_execFireAmmunition_Parms* params, void* result, Hooks::CallInfo callInfo);
void TrDevice_NovaSlug_FireAmmunition(ATrDevice_NovaSlug* that, ATrDevice_NovaSlug_execFireAmmunition_Parms* params, void* result, Hooks::CallInfo callInfo);

// Fix crashing/reconnecting causing players to end up with names like "Player256"
void UTGame_ChangeName(AUTGame* that, AUTGame_execChangeName_Parms* params);

// Server admin commands
extern int bNextMapOverrideValue;

// Reimplemented TakeDamage to revert Shield Pack behaviour
void TrPawn_TakeDamage(ATrPawn* that, ATrPawn_eventTakeDamage_Parms* params, void* result, Hooks::CallInfo* callinfo);
void TrPawn_GetUnshieldedDamage(ATrPawn* that, ATrPawn_execGetUnshieldedDamage_Parms* params, float* result);
void TrPawn_RememberLastDamager(ATrPawn* that, ATrPawn_execRememberLastDamager_Parms* params);
bool TrPawn_RechargeHealthPool(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult);
void TrPawn_GetLastDamager(ATrPawn* that, ATrPawn_execGetLastDamager_Parms* params, ATrPlayerController** result);
void TrPawn_ProcessKillAssists(ATrPawn* that, ATrPawn_execProcessKillAssists_Parms* params);
void TrPawn_SetShieldPackActive(ATrPawn* that, ATrPawn_execSetShieldPackActive_Parms* params);

// Reimplemented thrust pack acceleration to revert Rage behaviour
void TrPlayerController_GetBlinkPackAccel(ATrPlayerController* that, ATrPlayerController_execGetBlinkPackAccel_Parms* params, void* result, Hooks::CallInfo* callInfo);
void TrPlayerController_PlayerWalking_ProcessMove(ATrPlayerController* that, APlayerController_execProcessMove_Parms* params);

// Inv stations give energy
void TrPawn_RefreshInventory(ATrPawn* that, ATrPawn_execRefreshInventory_Parms* params);

// Fix honorfusor hitreg
void TrProj_Honorfusor_ProjectileHurtRadius(ATrProj_Honorfusor* that, ATrProj_Honorfusor_execProjectileHurtRadius_Parms* params, bool* result);

// BXT fixes
void TrDevice_SniperRifle_ModifyInstantHitDamage(ATrDevice_SniperRifle* that, ATrDevice_SniperRifle_execModifyInstantHitDamage_Parms* params, float* result, Hooks::CallInfo* callInfo);
bool TrDevice_SniperRifle_Tick(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult);
void ATrPlayerController_ResetZoomDuration(ATrPlayerController* that, ATrPlayerController_execResetZoomDuration_Parms* params, void* result, Hooks::CallInfo* callInfo);

void TrDevice_CalcHUDAimChargePercent(ATrDevice* that, ATrDevice_execCalcHUDAimChargePercent_Parms* params, float* result, Hooks::CallInfo* callInfo);
float TrDevice_LaserTargeter_CalcHUDAimChargePercent(ATrDevice_LaserTargeter* that);

// Inventory Stations
void TrDevice_LaserTargeter_OnStartConstantFire(ATrDevice_LaserTargeter* that, ATrDevice_LaserTargeter_execOnStartConstantFire_Parms* params, void* result, Hooks::CallInfo* callInfo);
void TrDevice_LaserTargeter_OnEndConstantFire(ATrDevice_LaserTargeter* that, ATrDevice_LaserTargeter_execOnEndConstantFire_Parms* params, void* result, Hooks::CallInfo* callInfo);
void TrDevice_LaserTargeter_GetLaserStartAndEnd(ATrDevice_LaserTargeter* that, ATrDevice_LaserTargeter_execGetLaserStartAndEnd_Parms* params, void* result, Hooks::CallInfo* callInfo);
void TrDevice_LaserTargeter_UpdateTarget(ATrDevice_LaserTargeter* that, ATrDevice_LaserTargeter_execUpdateTarget_Parms* params, void* result, Hooks::CallInfo* callInfo);
void TrDevice_LaserTargeter_SpawnLaserEffect(ATrDevice_LaserTargeter* that, ATrDevice_LaserTargeter_execSpawnLaserEffect_Parms* params, void* result, Hooks::CallInfo* callInfo);
void TrDevice_LaserTargeter_UpdateLaserEffect(ATrDevice_LaserTargeter* that, ATrDevice_LaserTargeter_execUpdateLaserEffect_Parms* params, void* result, Hooks::CallInfo* callInfo);
void ResetLaserTargetCallInCache();

void TrCallIn_SupportItemPlatform_Init(ATrCallIn_SupportItemPlatform* that, ATrCallIn_SupportItemPlatform_execInit_Parms* params);
void TrCallIn_SupportItemPlatform_HideMesh(ATrCallIn_SupportItemPlatform* that, ATrCallIn_SupportItemPlatform_execHideMesh_Parms* params);

void TrDevice_PlayWeaponEquip(ATrDevice* that, ATrDevice_execPlayWeaponEquip_Parms* params, void* result, Hooks::CallInfo* callInfo);
void TrDevice_UpdateWeaponMICs(ATrDevice* that, ATrDevice_eventUpdateWeaponMICs_Parms* params, void* result, Hooks::CallInfo* callInfo);

// Jackal airburst reimplementation
void ATrDevice_RemoteArxBuster_PerformInactiveReload(ATrDevice_RemoteArxBuster* that, ATrDevice_RemoteArxBuster_execPerformInactiveReload_Parms* params);
void ATrDevice_RemoteArxBuster_ProjectileFire(ATrDevice_RemoteArxBuster* that, ATrDevice_RemoteArxBuster_execProjectileFire_Parms* params, AProjectile** result);
void ATrDevice_RemoteArxBuster_ActivateRemoteRounds(ATrDevice_RemoteArxBuster* that, ATrDevice_RemoteArxBuster_execActivateRemoteRounds_Parms* params);

// Fix for ejection seats not working properly
void ATrVehicle_Died(ATrVehicle* that, ATrVehicle_execDied_Parms* params, bool* result);

// Vehicles cost credits
void TrVehicleStation_AbleToSpawnVehicleType(ATrVehicleStation* that, ATrVehicleStation_execAbleToSpawnVehicleType_Parms* params, bool* result);
bool TrPlayerController_ServerRequestSpawnVehicle(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult);

// Hooked StatsInterface functions for match screen
void TrStatsInterface_AddKill(UTrStatsInterface* that, UTrStatsInterface_execAddKill_Parms* params);
void TrStatsInterface_AddDeath(UTrStatsInterface* that, UTrStatsInterface_execAddDeath_Parms* params);
void TrStatsInterface_AddAssist(UTrStatsInterface* that, UTrStatsInterface_execAddAssist_Parms* params);
void TrStatsInterface_AddAccolade(UTrStatsInterface* that, UTrStatsInterface_execAddAccolade_Parms* params);
void TrStatsInterface_AddCredits(UTrStatsInterface* that, UTrStatsInterface_execAddCredits_Parms* params);
void TrStatsInterface_UpdateTimePlayed(UTrStatsInterface* that, UTrStatsInterface_execUpdateTimePlayed_Parms* params);
void TrStatsInterface_UpdateWeapon(UTrStatsInterface* that, UTrStatsInterface_execUpdateWeapon_Parms* params);
void TrStatsInterface_UpdateDamage(UTrStatsInterface* that, UTrStatsInterface_execUpdateDamage_Parms* params);

void TrStatsInterface_AddRepair(UTrStatsInterface* that, UTrStatsInterface_execAddRepair_Parms* params);
void TrStatsInterface_AddFlagCap(UTrStatsInterface* that, UTrStatsInterface_execAddFlagCap_Parms* params);
void TrStatsInterface_AddFlagGrab(UTrStatsInterface* that, UTrStatsInterface_execAddFlagGrab_Parms* params);
void TrStatsInterface_AddFlagReturn(UTrStatsInterface* that, UTrStatsInterface_execAddFlagReturn_Parms* params);
void TrStatsInterface_AddMidairKill(UTrStatsInterface* that, UTrStatsInterface_execAddMidairKill_Parms* params);
void TrStatsInterface_AddVehicleKill(UTrStatsInterface* that, UTrStatsInterface_execAddVehicleKill_Parms* params);
void TrStatsInterface_SetXP(UTrStatsInterface* that, UTrStatsInterface_execSetXP_Parms* params);
void TrStatsInterface_SetTeam(UTrStatsInterface* that, UTrStatsInterface_execSetTeam_Parms* params);
void TrStatsInterface_AddVehicleDestruction(UTrStatsInterface* that, UTrStatsInterface_execAddVehicleDestruction_Parms* params);
void TrStatsInterface_SetSpeedSkied(UTrStatsInterface* that, UTrStatsInterface_execSetSpeedSkied_Parms* params);
void TrStatsInterface_AddDeployableDestruction(UTrStatsInterface* that, UTrStatsInterface_execAddDeployableDestruction_Parms* params);
void TrStatsInterface_AddCreditsSpent(UTrStatsInterface* that, UTrStatsInterface_execAddCreditsSpent_Parms* params);
void TrStatsInterface_SetDistanceKill(UTrStatsInterface* that, UTrStatsInterface_execSetDistanceKill_Parms* params);
void TrStatsInterface_SetSpeedFlagCap(UTrStatsInterface* that, UTrStatsInterface_execSetSpeedFlagCap_Parms* params);
void TrStatsInterface_AddCreditsEarned(UTrStatsInterface* that, UTrStatsInterface_execAddCreditsEarned_Parms* params);
void TrStatsInterface_AddDistanceSkied(UTrStatsInterface* that, UTrStatsInterface_execAddDistanceSkied_Parms* params);
void TrStatsInterface_SetSpeedFlagGrab(UTrStatsInterface* that, UTrStatsInterface_execSetSpeedFlagGrab_Parms* params);
void TrStatsInterface_SetDistanceHeadshot(UTrStatsInterface* that, UTrStatsInterface_execSetDistanceHeadshot_Parms* params);

void TrStatsInterface_FallingDeath(UTrStatsInterface* that, UTrStatsInterface_execFallingDeath_Parms* params);
void TrStatsInterface_SkiDistance(UTrStatsInterface* that, UTrStatsInterface_execSkiDistance_Parms* params);
void TrStatsInterface_BeltKill(UTrStatsInterface* that, UTrStatsInterface_execBeltKill_Parms* params);
void TrStatsInterface_CallIn(UTrStatsInterface* that, UTrStatsInterface_execCallIn_Parms* params);
void TrStatsInterface_CallInKill(UTrStatsInterface* that, UTrStatsInterface_execCallInKill_Parms* params);
void TrStatsInterface_RegeneratedToFull(UTrStatsInterface* that, UTrStatsInterface_execRegeneratedToFull_Parms* params);
void TrStatsInterface_FlagGrabSpeed(UTrStatsInterface* that, UTrStatsInterface_execFlagGrabSpeed_Parms* params);
void TrStatsInterface_VehicleKill(UTrStatsInterface* that, UTrStatsInterface_execVEHICLEKILL_Parms* params);
void TrStatsInterface_InvStationVisited(UTrStatsInterface* that, UTrStatsInterface_execInvStationVisited_Parms* params);
void TrStatsInterface_SkiSpeed(UTrStatsInterface* that, UTrStatsInterface_execSkiSpeed_Parms* params);
void TrStatsInterface_BaseUpgrade(UTrStatsInterface* that, UTrStatsInterface_execBaseUpgrade_Parms* params);