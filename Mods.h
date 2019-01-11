#pragma once

#include <sstream>
#include <list>
#include <random>

#include "FunctionalUtils.h"

#include "TAServerClient.h"
#include "DirectConnectionServer.h"

#include "Config.h"
#include "Utils.h"
#include "Data.h"
#include "Geom.h"
#include "Logger.h"
#include "LuaAPI.h"
//#include "TAServerClient.h"


void GameInfo_ProcessServerTravel(AGameInfo* that, AGameInfo_execProcessServerTravel_Parms* params);

// Hook called on server settings load
void TrServerSettingsInfo_LoadServerSettings(ATrServerSettingsInfo* that, ATrServerSettingsInfo_eventLoadServerSettings_Parms* params, void* result, Hooks::CallInfo* callInfo);

// Hook called to process the swapping to a new class
//void TrPlayerReplicationInfo_SwapToPendingCharClass(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execSwapToPendingCharClass_Parms* params, void* result, Hooks::CallInfo* callInfo);
// Hook called when the server tries to retrieve a player's loadout
void TrPlayerReplicationInfo_GetCharacterEquip(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execGetCharacterEquip_Parms* params, void* result, Hooks::CallInfo* callInfo);
// Hook called when the server tries to fill in gaps in a player's loadout with defaults
void TrPlayerReplicationInfo_ResolveDefaultEquip(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execResolveDefaultEquip_Parms* params, void* result, Hooks::CallInfo* callInfo);
// Hook called when the game tries to assign the player their voice; used because it happens after equipment is resolved
void TrPlayerReplicationInfo_GetCurrentVoiceClass(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execGetCurrentVoiceClass_Parms* params, UClass** result, Hooks::CallInfo* callInfo);

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

// Server admin commands
extern int bNextMapOverrideValue;

// Reimplemented TakeDamage to revert Shield Pack behaviour
void TrPawn_TakeDamage(ATrPawn* that, ATrPawn_eventTakeDamage_Parms* params, void* result, Hooks::CallInfo* callinfo);

// Reimplemented thrust pack acceleration to revert Rage behaviour
void TrPlayerController_GetBlinkPackAccel(ATrPlayerController* that, ATrPlayerController_execGetBlinkPackAccel_Parms* params);
void TrDevice_Blink_OnBlink(ATrDevice_Blink* that, ATrDevice_Blink_execOnBlink_Parms* params);

// Inv stations give energy
void TrPawn_RefreshInventory(ATrPawn* that, ATrPawn_execRefreshInventory_Parms* params);

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