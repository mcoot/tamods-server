// dllmain.cpp : Defines the entry point for the DLL application.
#include "dllmain.h"
#include <shellapi.h>

static void fixPingDependencies() {
    std::vector<UClass*> toFix = {
        ATrDevice_NovaSlug::StaticClass(),

        // Not working?
        ATrDevice_Shotgun::StaticClass(),
        ATrDevice_Shotgun_MKD::StaticClass(),
        ATrDevice_AccurizedShotgun::StaticClass(),
        ATrDevice_SawedOffShotgun::StaticClass(),
        ATrDevice_AutoShotgun::StaticClass(),
        ATrDevice_AutoShotgun_MKD::StaticClass(),

        ATrDevice_LightTwinfusor::StaticClass(),
        ATrDevice_Twinfusor::StaticClass(),
        ATrDevice_HeavyTwinfusor::StaticClass(),
        ATrDevice_SniperRifle::StaticClass(),
        ATrDevice_SniperRifle_MKD::StaticClass(),
        ATrDevice_PhaseRifle::StaticClass(),
        ATrDevice_SAP20::StaticClass(),
        ATrDevice_GrenadeLauncher::StaticClass(),
        ATrDevice_GrenadeLauncher_Light::StaticClass(),
        ATrDevice_ArxBuster::StaticClass(),
        ATrDevice_ArxBuster_MKD::StaticClass(),
        ATrDevice_RemoteArxBuster::StaticClass(),
        ATrDevice_PlasmaGun::StaticClass(),
        ATrDevice_PlasmaCannon::StaticClass(),
    };

    for (auto& cls : toFix) {
        ATrDevice* default = (ATrDevice*)cls->Default;
        default->m_bForceReplicateAmmoOnFire = true;
    }
}

static void testing_PrintOutGObjectIndices() {
    for (int i = 0; i < UObject::GObjObjects()->Count; ++i)
    {
        UObject* Object = UObject::GObjObjects()->Data[i];
        Logger::debug("%d \t\t | %s", i, Object->GetFullName());
    }
}

static bool GenericLoggingEventHook(int ID, UObject *dwCallingObject, UFunction* pFunction, void* pParams, void* pResult) {
    std::string fName = pFunction ? pFunction->Name.GetName() : "<null ufunction>";
    std::string oName = dwCallingObject ? dwCallingObject->Name.GetName() : "<null uobject>";
    Logger::debug("Event hook call | Object %s | Function: %s", oName.c_str(), fName.c_str());
    return false;
}

static void GenericLoggingUScriptHook(UObject* that, void* params, void* result, Hooks::CallInfo* callInfo) {
    std::string callerObject = callInfo->callerObject ? callInfo->callerObject->Name.GetName() : "<null>";
    std::string callerFunc = callInfo->caller ? callInfo->caller->Name.GetName() : "<null>";
    std::string calleeObject = callInfo->calleeObject ? callInfo->calleeObject->Name.GetName() : "<null>";
    std::string calleeFunc = callInfo->callee ? callInfo->callee->Name.GetName() : "<null>";

    Logger::debug("UScript hook call | Caller: %s.%s | Callee: %s.%s", callerObject.c_str(), callerFunc.c_str(), calleeObject.c_str(), calleeFunc.c_str());
    Logger::debug("<UScript hook blackholed>");
}

static void addStatsFunctionHooks() {
    Hooks::addUScript(&TrStatsInterface_AddKill, "Function TribesGame.TrStatsInterface.AddKill");
    Hooks::addUScript(&TrStatsInterface_AddDeath, "Function TribesGame.TrStatsInterface.AddDeath");
    Hooks::addUScript(&TrStatsInterface_AddAssist, "Function TribesGame.TrStatsInterface.AddAssist");
    Hooks::addUScript(&TrStatsInterface_AddAccolade, "Function TribesGame.TrStatsInterface.AddAccolade");
    Hooks::addUScript(&TrStatsInterface_AddCredits, "Function TribesGame.TrStatsInterface.AddCredits");
    Hooks::addUScript(&TrStatsInterface_UpdateTimePlayed, "Function TribesGame.TrStatsInterface.UpdateTimePlayed");
    Hooks::addUScript(&TrStatsInterface_UpdateWeapon, "Function TribesGame.TrStatsInterface.UpdateWeapon");
    Hooks::addUScript(&TrStatsInterface_UpdateDamage, "Function TribesGame.TrStatsInterface.UpdateDamage");

    Hooks::addUScript(&TrStatsInterface_AddRepair, "Function TribesGame.TrStatsInterface.AddRepair");
    Hooks::addUScript(&TrStatsInterface_AddFlagCap, "Function TribesGame.TrStatsInterface.AddFlagCap");
    Hooks::addUScript(&TrStatsInterface_AddFlagGrab, "Function TribesGame.TrStatsInterface.AddFlagGrab");
    Hooks::addUScript(&TrStatsInterface_AddFlagReturn, "Function TribesGame.TrStatsInterface.AddFlagReturn");
    Hooks::addUScript(&TrStatsInterface_AddMidairKill, "Function TribesGame.TrStatsInterface.AddMidairKill");
    Hooks::addUScript(&TrStatsInterface_AddVehicleKill, "Function TribesGame.TrStatsInterface.AddVehicleKill");
    Hooks::addUScript(&TrStatsInterface_SetXP, "Function TribesGame.TrStatsInterface.SetXP");
    Hooks::addUScript(&TrStatsInterface_SetTeam, "Function TribesGame.TrStatsInterface.SetTeam");
    Hooks::addUScript(&TrStatsInterface_AddVehicleDestruction, "Function TribesGame.TrStatsInterface.AddVehicleDestruction");
    Hooks::addUScript(&TrStatsInterface_SetSpeedSkied, "Function TribesGame.TrStatsInterface.SetSpeedSkied");
    Hooks::addUScript(&TrStatsInterface_AddDeployableDestruction, "Function TribesGame.TrStatsInterface.AddDeployableDestruction");
    Hooks::addUScript(&TrStatsInterface_AddCreditsSpent, "Function TribesGame.TrStatsInterface.AddCreditsSpent");
    Hooks::addUScript(&TrStatsInterface_SetDistanceKill, "Function TribesGame.TrStatsInterface.SetDistanceKill");
    Hooks::addUScript(&TrStatsInterface_SetSpeedFlagCap, "Function TribesGame.TrStatsInterface.SetSpeedFlagCap");
    Hooks::addUScript(&TrStatsInterface_AddCreditsEarned, "Function TribesGame.TrStatsInterface.AddCreditsEarned");
    Hooks::addUScript(&TrStatsInterface_AddDistanceSkied, "Function TribesGame.TrStatsInterface.AddDistanceSkied");
    Hooks::addUScript(&TrStatsInterface_SetSpeedFlagGrab, "Function TribesGame.TrStatsInterface.SetSpeedFlagGrab");
    Hooks::addUScript(&TrStatsInterface_SetDistanceHeadshot, "Function TribesGame.TrStatsInterface.SetDistanceHeadshot");

    Hooks::addUScript(&TrStatsInterface_FallingDeath, "Function TribesGame.TrStatsInterface.FallingDeath");
    Hooks::addUScript(&TrStatsInterface_SkiDistance, "Function TribesGame.TrStatsInterface.SkiDistance");
    Hooks::addUScript(&TrStatsInterface_BeltKill, "Function TribesGame.TrStatsInterface.BeltKill");
    Hooks::addUScript(&TrStatsInterface_CallIn, "Function TribesGame.TrStatsInterface.CallIn");
    Hooks::addUScript(&TrStatsInterface_CallInKill, "Function TribesGame.TrStatsInterface.CallInKill");
    Hooks::addUScript(&TrStatsInterface_RegeneratedToFull, "Function TribesGame.TrStatsInterface.RegeneratedToFull");
    Hooks::addUScript(&TrStatsInterface_FlagGrabSpeed, "Function TribesGame.TrStatsInterface.FlagGrabSpeed");
    // TODO: Verify if below needs to be VEHICLEKILL uppercase - the generated SDK has this for some reason?
    // Requires testing in-game if the hook works
    Hooks::addUScript(&TrStatsInterface_VehicleKill, "Function TribesGame.TrStatsInterface.VehicleKill");
    Hooks::addUScript(&TrStatsInterface_InvStationVisited, "Function TribesGame.TrStatsInterface.InvStationVisited");
    Hooks::addUScript(&TrStatsInterface_SkiSpeed, "Function TribesGame.TrStatsInterface.SkiSpeed");
    Hooks::addUScript(&TrStatsInterface_BaseUpgrade, "Function TribesGame.TrStatsInterface.BaseUpgrade");
}

// Desired hooks should be added here
static void addHooks() {
    // Disabled due to bugs
    //fixPingDependencies();

    //UClass* havocClass = UObject::FindClass("Class TribesGame.TrVehicle_Havoc");
    //ATrVehicle_Havoc* havoc = (ATrVehicle_Havoc*)havocClass->Default;
    //Logger::debug("Havocy: %p", havoc);
    //havoc->m_VehicleType = VEHICLE_Havoc;
    //havoc->m_sName = L"Havoc";
    //Logger::debug("uwu");

    //Hooks::addUScript(&GenericLoggingUScriptHook, "Function TribesGame.TrGameObjective.PerformUpgrade");

    Hooks::addUScript(&TrDevice_SniperRifle_ModifyInstantHitDamage, "Function TribesGame.TrDevice_SniperRifle.ModifyInstantHitDamage");
    Hooks::add(&TrDevice_SniperRifle_Tick, "Function TribesGame.TrDevice_SniperRifle.Tick");
    Hooks::addUScript(&ATrPlayerController_ResetZoomDuration, "Function TribesGame.TrPlayerController.ResetZoomDuration");

    Hooks::addUScript(&TrDevice_CalcHUDAimChargePercent, "Function TribesGame.TrDevice.CalcHUDAimChargePercent");
    Hooks::addUScript(&TrDevice_LaserTargeter_OnStartConstantFire, "Function TribesGame.TrDevice_LaserTargeter.OnStartConstantFire");
    Hooks::addUScript(&TrDevice_LaserTargeter_OnEndConstantFire, "Function TribesGame.TrDevice_LaserTargeter.OnEndConstantFire");
    Hooks::addUScript(&TrDevice_LaserTargeter_UpdateTarget, "Function TribesGame.TrDevice_LaserTargeter.UpdateTarget");
    //Hooks::addUScript(&TrDevice_LaserTargeter_GetLaserStartAndEnd, "Function TribesGame.TrDevice_LaserTargeter.GetLaserStartAndEnd");
    //Hooks::addUScript(&TrDevice_LaserTargeter_SpawnLaserEffect, "Function TribesGame.TrDevice_LaserTargeter.SpawnLaserEffect");
    Hooks::addUScript(&TrDevice_LaserTargeter_UpdateLaserEffect, "Function TribesGame.TrDevice_LaserTargeter.UpdateLaserEffect");
    Hooks::addUScript(&TrCallIn_SupportItemPlatform_Init, "Function TribesGame.TrCallIn_SupportItemPlatform.Init");
    // WIP
    //Hooks::addUScript(&TrCallIn_SupportItemPlatform_HideMesh, "Function TribesGame.TrCallIn_SupportItemPlatform.HideMesh");

    //Hooks::addUScript(&TrDevice_PlayWeaponEquip, "Function TribesGame.TrDevice.PlayWeaponEquip");
    //Hooks::addUScript(&TrDevice_UpdateWeaponMICs, "Function TribesGame.TrDevice.UpdateWeaponMICs");

    // Server Settings
    Hooks::addUScript(&TrServerSettingsInfo_LoadServerSettings, "Function TribesGame.TrServerSettingsInfo.LoadServerSettings");
    Hooks::addUScript(&TrPlayerController_GetRespawnDelayTotalTime, "Function TribesGame.TrPlayerController.GetRespawnDelayTotalTime");

    // Loadouts
    Hooks::addUScript(&TrPlayerReplicationInfo_VerifyCharacter, "Function TribesGame.TrPlayerReplicationInfo.VerifyCharacter");
    Hooks::addUScript(&TrPlayerReplicationInfo_GetCharacterEquip, "Function TribesGame.TrPlayerReplicationInfo.GetCharacterEquip");
    Hooks::addUScript(&TrPlayerReplicationInfo_ResolveDefaultEquip, "Function TribesGame.TrPlayerReplicationInfo.ResolveDefaultEquip");
    // TODO: Delete all the logic and config stuff associated with the previous custom class feature implementation
    //Hooks::addUScript(&TrPlayerReplicationInfo_GetCurrentVoiceClass, "Function TribesGame.TrPlayerReplicationInfo.GetCurrentVoiceClass");
    Hooks::addUScript(&TrPlayerController_GetFamilyInfoFromId, "Function TribesGame.TrPlayerController.GetFamilyInfoFromId");
    //Hooks::addUScript(&TrDevice_GetReloadTime, "Function TribesGame.TrDevice.GetReloadTime");

    // TAServer Game Info
    Hooks::add(&TrGameReplicationInfo_PostBeginPlay, "Function TribesGame.TrGameReplicationInfo.PostBeginPlay");
    Hooks::add(&TrGameReplicationInfo_Tick, "Function TribesGame.TrGameReplicationInfo.Tick");
    //Hooks::addUScript(&TrGame_SendShowSummary, "Function TribesGame.TrGame.SendShowSummary");
    Hooks::add(&UTGame_MatchInProgress_BeginState, "Function UTGame.MatchInProgress.BeginState");
    Hooks::addUScript(&TrGame_RequestTeam, "Function TribesGame.TrGame.RequestTeam");
    Hooks::addUScript(&UTGame_EndGame, "Function UTGame.UTGame.EndGame");
    Hooks::addUScript(&PlayerController_ServerUpdatePing, "Function Engine.PlayerController.ServerUpdatePing");

    Hooks::addUScript(&UTGame_ChangeName, "Function UTGame.UTGame.ChangeName");

    // Part of fix for colt ping dependency
    Hooks::addUScript(&TrDevice_FireAmmunition, "Function TribesGame.TrDevice.FireAmmunition");
    Hooks::addUScript(&TrDevice_NovaSlug_FireAmmunition, "Function TribesGame.TrDevice_NovaSlug.FireAmmunition");

    Hooks::addUScript(&UTGame_ProcessServerTravel, "Function UTGame.UTGame.ProcessServerTravel");

    Hooks::addUScript(&TrPawn_TakeDamage, "Function TribesGame.TrPawn.TakeDamage");
    Hooks::addUScript(&TrPawn_GetUnshieldedDamage, "Function TribesGame.TrPawn.GetUnshieldedDamage");
    Hooks::addUScript(&TrPawn_RememberLastDamager, "Function TribesGame.TrPawn.RememberLastDamager");
    Hooks::add(&TrPawn_RechargeHealthPool, "Function TribesGame.TrPawn.RechargeHealthPool");
    Hooks::addUScript(&TrPawn_GetLastDamager, "Function TribesGame.TrPawn.GetLastDamager");
    Hooks::addUScript(&TrPawn_ProcessKillAssists, "Function TribesGame.TrPawn.ProcessKillAssists");
    Hooks::addUScript(&TrPawn_SetShieldPackActive, "Function TribesGame.TrPawn.SetShieldPackActive");
    //Hooks::addUScript(&TrPawn_Died, "Function TribesGame.TrPawn.Died");

    Hooks::addUScript(&TrPawn_RefreshInventory, "Function TribesGame.TrPawn.RefreshInventory");

    //Hooks::addUScript(&ATrDevice_RemoteArxBuster_ActivateRemoteRounds, "Function TribesGame.TrDevice_RemoteArxBuster.ActivateRemoteRounds");

    // Honorfusor hitreg issues
    ATrProj_Honorfusor* honorfusorDefault = (ATrProj_Honorfusor*)UObject::FindObject<UObject>("TrProj_Honorfusor TribesGame.Default__TrProj_Honorfusor");
    //Logger::debug("m_bIsBullet = %d", honorfusorDefault->m_bIsBullet);
    //Logger::debug("DamageRadius = %f", honorfusorDefault->DamageRadius);
    //Logger::debug("CheckRadius = %f", honorfusorDefault->CheckRadius);
    //honorfusorDefault->m_bIsBullet = true;
    //honorfusorDefault->DamageRadius = 100;
    //Hooks::addUScript(&TrProj_Honorfusor_ProjectileHurtRadius, "Function TribesGame.TrProj_Honorfusor.ProjectileHurtRadius");

    // Rage pack capper speed dependency fix
    Hooks::addUScript(&TrPlayerController_PlayerWalking_ProcessMove, "Function TrPlayerController.PlayerWalking.ProcessMove");

    // Vehicle ejection seat fix
    Hooks::addUScript(&ATrVehicle_Died, "Function TribesGame.TrVehicle.Died");

    // Vehicles cost credits
    Hooks::addUScript(&TrVehicleStation_AbleToSpawnVehicleType, "Function TribesGame.TrVehicleStation.AbleToSpawnVehicleType");
    Hooks::add(&TrPlayerController_ServerRequestSpawnVehicle, "Function TribesGame.TrPlayerController.ServerRequestSpawnVehicle");

    addStatsFunctionHooks();
}

#define CLI_FLAG_CONTROLPORT L"-controlport"

int determineControlPort() {
    int numArgs;
    wchar_t** args = CommandLineToArgvW(GetCommandLineW(), &numArgs);

    if (args) {
        // Only check up to the second last argument
        // Since we need both the flag and the path
        for (int i = 0; i < numArgs - 1; ++i) {
            if (wcscmp(args[i], CLI_FLAG_CONTROLPORT) == 0) {
                // This won't work if there are non-ascii characters in the path but yolo
                std::wstring portWString(args[i + 1]);
                std::string portString(portWString.begin(), portWString.end());
                return std::stoi(portString);
            }
        }
    }
    return 9002;
}

void onDLLProcessAttach() {
    
#ifdef RELEASE
    Logger::setLevel(Logger::LOG_ERROR);
#else
    Logger::setLevel(Logger::LOG_DEBUG);
#endif
    Logger::info("Initialising...");

    g_config.load();
    Logger::info("Configuration loaded");
    if (g_config.eventLogging) {
        EventLogger::init();
    }

    //testing_PrintOutGObjectIndices();

    addHooks();

    Hooks::init(false);

    //g_config.serverSettings.ApplyAsDefaults();

    if (g_config.connectToTAServer) {
        int controlPort = determineControlPort();

        Logger::info("Connecting to TAServer on port %d...", controlPort);

        if (g_TAServerClient.connect("127.0.0.1", controlPort)) {
            Logger::debug("Connected to TAServer!");
        } else {
            Logger::error("Failed to connect to TAServer launcher.");
        }
    }
    else {
        Logger::info("Not attempting connection");
    }
}

void onDLLProcessDetach() {
    if (g_TAServerClient.isConnected()) {
        if (!g_TAServerClient.disconnect()) {
            Logger::error("Failed to disconnect from TAServer launcher.");
        }
    }
    Logger::debug("DLL Process Detach");
    Hooks::cleanup();
    Logger::cleanup();
    EventLogger::cleanup();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)onDLLProcessAttach, NULL, 0, NULL);
        break;
    case DLL_PROCESS_DETACH:
        onDLLProcessDetach();
        break;
    }
    return TRUE;
}

