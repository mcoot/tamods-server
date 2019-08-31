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


void GameInfo_ProcessServerTravel(AGameInfo* that, AGameInfo_execProcessServerTravel_Parms* params);

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
void TrPawn_RememberLastDamager(ATrPawn* that, ATrPawn_execRememberLastDamager_Parms* params);
bool TrPawn_RechargeHealthPool(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult);
void TrPawn_GetLastDamager(ATrPawn* that, ATrPawn_execGetLastDamager_Parms* params, ATrPlayerController** result);
void TrPawn_ProcessKillAssists(ATrPawn* that, ATrPawn_execProcessKillAssists_Parms* params);

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