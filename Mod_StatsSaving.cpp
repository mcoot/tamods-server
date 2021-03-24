#include "MatchSummary.h"
#include "Mods.h"

// TODO: in functions with TAMods re-implementations, ensure these stats functions get called instead of the originals!

// Updating stats in this list overrides the value, for other stats it adds to the existing value
static std::vector<int> absoluteStats = {
    CONST_STAT_AWD_SPEED_SKIED,
    CONST_STAT_AWD_DISTANCE_KILL,
    CONST_STAT_AWD_SPEED_FLAGCAP,
    CONST_STAT_AWD_DISTANCE_SKIED,
    CONST_STAT_AWD_SPEED_FLAGGRAB,
    CONST_STAT_AWD_DISTANCE_HEADSHOT,
};

static void updateStat(ATrPlayerController* pc, int statId, int value)
{
    if(pc)
    {
        auto pri = (ATrPlayerReplicationInfo *)pc->PlayerReplicationInfo;
        auto playerId = pri->UniqueId.Uid.A;

        MatchSummary::sStatsCollector.updateStat(playerId, statId, (float)value);
#ifdef _DEBUG
        Logger::error("updateStat(playercontroller = 0x%X, stat = %d, value = %d)", playerId, statId, value);
#endif
    }
    else
    {
#ifdef _DEBUG
        Logger::error("updateStat(NULL, stat = %d, value = %d)", statId, value);
#endif
    }
}

static void addToStat(ATrPlayerController* pc, int statId, int value)
{
    if(pc)
    {
        auto pri = (ATrPlayerReplicationInfo *)pc->PlayerReplicationInfo;
        auto playerId = pri->UniqueId.Uid.A;

        MatchSummary::sStatsCollector.addToStat(playerId, statId, (float)value);
#ifdef _DEBUG
        Logger::error("addToStat(playercontroller = 0x%X, stat = %d, value = %d)", playerId, statId, value);
#endif
    }
    else
    {
#ifdef _DEBUG
        Logger::error("addToStat(NULL, stat = %d, value = %d)", statId, value);
#endif
    }
}

static void addAccolade(ATrPlayerController* pc, int accoladeId)
{
    if(pc)
    {
        auto pri = (ATrPlayerReplicationInfo *)pc->PlayerReplicationInfo;
        auto playerId = pri->UniqueId.Uid.A;

        MatchSummary::sStatsCollector.addAccolade(playerId, accoladeId);
#ifdef _DEBUG
        Logger::error("addAccolade(playercontroller = 0x%X, stat = %d)", playerId, accoladeId);
#endif
    }
    else
    {
#ifdef _DEBUG
        Logger::error("addAccolade(NULL, stat = %d)", accoladeId);
#endif
    }
}

// Hooked stats functions

// TODO: Are the STAT_CLASS_* the right ids?
void TrStatsInterface_AddKill(UTrStatsInterface* that, UTrStatsInterface_execAddKill_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_KILLS, 1);
}

void TrStatsInterface_AddDeath(UTrStatsInterface* that, UTrStatsInterface_execAddDeath_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_DEATHS, 1);
}

void TrStatsInterface_AddAssist(UTrStatsInterface* that, UTrStatsInterface_execAddAssist_Parms* params) {
    addToStat(params->PC, CONST_STAT_CLASS_ASSISTS, 1); // No AWD for this
}

void TrStatsInterface_AddAccolade(UTrStatsInterface* that, UTrStatsInterface_execAddAccolade_Parms* params) {
    addAccolade(params->PC, params->Id);
}

void TrStatsInterface_AddCredits(UTrStatsInterface* that, UTrStatsInterface_execAddCredits_Parms* params) {
    // TODO: unnecessary given TrStatsInterface_AddCreditsEarned / TrStatsInterface_AddCreditsSpent?
    //if (params->val > 0) {
    //    updateStat(params->PC, CONST_STAT_CLASS_CREDITS_EARNED, params->val);
    //}
    //else {
    //    updateStat(params->PC, CONST_STAT_CLASS_CREDITS_SPENT, -params->val);
    //}
}

void TrStatsInterface_UpdateTimePlayed(UTrStatsInterface* that, UTrStatsInterface_execUpdateTimePlayed_Parms* params) {
    // I think this is only used for player profile, not for match summary
}

void TrStatsInterface_UpdateWeapon(UTrStatsInterface* that, UTrStatsInterface_execUpdateWeapon_Parms* params) {
    // I think this is only used for player profile, not for match summary
}

void TrStatsInterface_UpdateDamage(UTrStatsInterface* that, UTrStatsInterface_execUpdateDamage_Parms* params) {
    // I think this is only used for player profile, not for match summary
}

void TrStatsInterface_AddRepair(UTrStatsInterface* that, UTrStatsInterface_execAddRepair_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_REPAIRS, 1);
}

void TrStatsInterface_AddFlagCap(UTrStatsInterface* that, UTrStatsInterface_execAddFlagCap_Parms* params) {
    addToStat(params->PC, CONST_STAT_ACO_FLAG_CAPTURE, 1); // No AWD for this
}

void TrStatsInterface_AddFlagGrab(UTrStatsInterface* that, UTrStatsInterface_execAddFlagGrab_Parms* params) {
    addToStat(params->PC, CONST_STAT_ACO_FLAG_GRAB, 1); // No AWD for this
}

void TrStatsInterface_AddFlagReturn(UTrStatsInterface* that, UTrStatsInterface_execAddFlagReturn_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_FLAG_RETURNS, 1);
}

void TrStatsInterface_AddMidairKill(UTrStatsInterface* that, UTrStatsInterface_execAddMidairKill_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_KILLS_MIDAIR, 1);
}

void TrStatsInterface_AddVehicleKill(UTrStatsInterface* that, UTrStatsInterface_execAddVehicleKill_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_KILLS_VEHICLE, 1);
}

void TrStatsInterface_SetXP(UTrStatsInterface* that, UTrStatsInterface_execSetXP_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_SetTeam(UTrStatsInterface* that, UTrStatsInterface_execSetTeam_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_AddVehicleDestruction(UTrStatsInterface* that, UTrStatsInterface_execAddVehicleDestruction_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_DESTRUCTION_VEHICLE, 1);
}

void TrStatsInterface_SetSpeedSkied(UTrStatsInterface* that, UTrStatsInterface_execSetSpeedSkied_Parms* params) {
    updateStat(params->PC, CONST_STAT_AWD_SPEED_SKIED, params->val);
}

void TrStatsInterface_AddDeployableDestruction(UTrStatsInterface* that, UTrStatsInterface_execAddDeployableDestruction_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_DESTRUCTION_DEPLOYABLE, 1);
}

void TrStatsInterface_AddCreditsSpent(UTrStatsInterface* that, UTrStatsInterface_execAddCreditsSpent_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_CREDITS_SPENT, params->val);
}

void TrStatsInterface_SetDistanceKill(UTrStatsInterface* that, UTrStatsInterface_execSetDistanceKill_Parms* params) {
    updateStat(params->PC, CONST_STAT_AWD_DISTANCE_KILL, params->val);
}

void TrStatsInterface_SetSpeedFlagCap(UTrStatsInterface* that, UTrStatsInterface_execSetSpeedFlagCap_Parms* params) {
    updateStat(params->PC, CONST_STAT_AWD_SPEED_FLAGCAP, params->val);
}

void TrStatsInterface_AddCreditsEarned(UTrStatsInterface* that, UTrStatsInterface_execAddCreditsEarned_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_CREDITS_EARNED, params->val);
}

void TrStatsInterface_AddDistanceSkied(UTrStatsInterface* that, UTrStatsInterface_execAddDistanceSkied_Parms* params) {
    addToStat(params->PC, CONST_STAT_AWD_DISTANCE_SKIED, params->val);
}

void TrStatsInterface_SetSpeedFlagGrab(UTrStatsInterface* that, UTrStatsInterface_execSetSpeedFlagGrab_Parms* params) {
    updateStat(params->PC, CONST_STAT_AWD_SPEED_FLAGGRAB, params->val);
}

void TrStatsInterface_SetDistanceHeadshot(UTrStatsInterface* that, UTrStatsInterface_execSetDistanceHeadshot_Parms* params) {
    updateStat(params->PC, CONST_STAT_AWD_DISTANCE_HEADSHOT, params->val);
}

void TrStatsInterface_FallingDeath(UTrStatsInterface* that, UTrStatsInterface_execFallingDeath_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_SkiDistance(UTrStatsInterface* that, UTrStatsInterface_execSkiDistance_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_BeltKill(UTrStatsInterface* that, UTrStatsInterface_execBeltKill_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_CallIn(UTrStatsInterface* that, UTrStatsInterface_execCallIn_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_CallInKill(UTrStatsInterface* that, UTrStatsInterface_execCallInKill_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_RegeneratedToFull(UTrStatsInterface* that, UTrStatsInterface_execRegeneratedToFull_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_FlagGrabSpeed(UTrStatsInterface* that, UTrStatsInterface_execFlagGrabSpeed_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_VehicleKill(UTrStatsInterface* that, UTrStatsInterface_execVEHICLEKILL_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_InvStationVisited(UTrStatsInterface* that, UTrStatsInterface_execInvStationVisited_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_SkiSpeed(UTrStatsInterface* that, UTrStatsInterface_execSkiSpeed_Parms* params) {
    // No relevant stat
}

void TrStatsInterface_BaseUpgrade(UTrStatsInterface* that, UTrStatsInterface_execBaseUpgrade_Parms* params) {
    // No relevant stat
}