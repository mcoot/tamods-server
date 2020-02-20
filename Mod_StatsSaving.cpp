#include "MatchSummary.h"
#include "Mods.h"

// TODO: in functions with TAMods re-implementations, ensure these stats functions get called instead of the originals!

template <typename T>
static void updateStat(ATrPlayerController* pc, int statId, T value) {
    // TODO: Update the stat
}

static void addAccolade(ATrPlayerController* pc, int accoladeId) {
    // TODO: Save the accolade
}

// Hooked stats functions

// TODO: Are the STAT_CLASS_* the right ids?
void TrStatsInterface_AddKill(UTrStatsInterface* that, UTrStatsInterface_execAddKill_Parms* params) {
    updateStat(params->PC, CONST_STAT_CLASS_KILLS, 1);
}

void TrStatsInterface_AddDeath(UTrStatsInterface* that, UTrStatsInterface_execAddDeath_Parms* params) {
    updateStat(params->PC, CONST_STAT_CLASS_DEATHS, 1);
}

void TrStatsInterface_AddAssist(UTrStatsInterface* that, UTrStatsInterface_execAddAssist_Parms* params) {
    updateStat(params->PC, CONST_STAT_CLASS_ASSISTS, 1);
}

void TrStatsInterface_AddAccolade(UTrStatsInterface* that, UTrStatsInterface_execAddAccolade_Parms* params) {
    addAccolade(params->PC, params->Id);
}

void TrStatsInterface_AddCredits(UTrStatsInterface* that, UTrStatsInterface_execAddCredits_Parms* params) {
    if (params->val > 0) {
        updateStat(params->PC, CONST_STAT_CLASS_CREDITS_EARNED, params->val);
    }
    else {
        updateStat(params->PC, CONST_STAT_CLASS_CREDITS_SPENT, -params->val);
    }
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
    
}

void TrStatsInterface_AddFlagCap(UTrStatsInterface* that, UTrStatsInterface_execAddFlagCap_Parms* params) {

}

void TrStatsInterface_AddFlagGrab(UTrStatsInterface* that, UTrStatsInterface_execAddFlagGrab_Parms* params) {

}

void TrStatsInterface_AddFlagReturn(UTrStatsInterface* that, UTrStatsInterface_execAddFlagReturn_Parms* params) {

}

void TrStatsInterface_AddMidairKill(UTrStatsInterface* that, UTrStatsInterface_execAddMidairKill_Parms* params) {

}

void TrStatsInterface_AddVehicleKill(UTrStatsInterface* that, UTrStatsInterface_execAddVehicleKill_Parms* params) {

}

void TrStatsInterface_SetXP(UTrStatsInterface* that, UTrStatsInterface_execSetXP_Parms* params) {

}

void TrStatsInterface_SetTeam(UTrStatsInterface* that, UTrStatsInterface_execSetTeam_Parms* params) {

}

void TrStatsInterface_AddVehicleDestruction(UTrStatsInterface* that, UTrStatsInterface_execAddVehicleDestruction_Parms* params) {

}

void TrStatsInterface_SetSpeedSkied(UTrStatsInterface* that, UTrStatsInterface_execSetSpeedSkied_Parms* params) {

}

void TrStatsInterface_AddDeployableDestruction(UTrStatsInterface* that, UTrStatsInterface_execAddDeployableDestruction_Parms* params) {

}

void TrStatsInterface_AddCreditsSpent(UTrStatsInterface* that, UTrStatsInterface_execAddCreditsSpent_Parms* params) {

}

void TrStatsInterface_SetDistanceKill(UTrStatsInterface* that, UTrStatsInterface_execSetDistanceKill_Parms* params) {

}

void TrStatsInterface_SetSpeedFlagCap(UTrStatsInterface* that, UTrStatsInterface_execSetSpeedFlagCap_Parms* params) {

}

void TrStatsInterface_AddCreditsEarned(UTrStatsInterface* that, UTrStatsInterface_execAddCreditsEarned_Parms* params) {

}

void TrStatsInterface_AddDistanceSkied(UTrStatsInterface* that, UTrStatsInterface_execAddDistanceSkied_Parms* params) {

}

void TrStatsInterface_SetSpeedFlagGrab(UTrStatsInterface* that, UTrStatsInterface_execSetSpeedFlagGrab_Parms* params) {

}

void TrStatsInterface_SetDistanceHeadshot(UTrStatsInterface* that, UTrStatsInterface_execSetDistanceHeadshot_Parms* params) {

}

void TrStatsInterface_FallingDeath(UTrStatsInterface* that, UTrStatsInterface_execFallingDeath_Parms* params) {

}

void TrStatsInterface_SkiDistance(UTrStatsInterface* that, UTrStatsInterface_execSkiDistance_Parms* params) {

}

void TrStatsInterface_BeltKill(UTrStatsInterface* that, UTrStatsInterface_execBeltKill_Parms* params) {

}

void TrStatsInterface_CallIn(UTrStatsInterface* that, UTrStatsInterface_execCallIn_Parms* params) {

}

void TrStatsInterface_CallInKill(UTrStatsInterface* that, UTrStatsInterface_execCallInKill_Parms* params) {

}

void TrStatsInterface_RegeneratedToFull(UTrStatsInterface* that, UTrStatsInterface_execRegeneratedToFull_Parms* params) {

}

void TrStatsInterface_FlagGrabSpeed(UTrStatsInterface* that, UTrStatsInterface_execFlagGrabSpeed_Parms* params) {

}

void TrStatsInterface_VehicleKill(UTrStatsInterface* that, UTrStatsInterface_execVEHICLEKILL_Parms* params) {

}

void TrStatsInterface_InvStationVisited(UTrStatsInterface* that, UTrStatsInterface_execInvStationVisited_Parms* params) {

}

void TrStatsInterface_SkiSpeed(UTrStatsInterface* that, UTrStatsInterface_execSkiSpeed_Parms* params) {

}

void TrStatsInterface_BaseUpgrade(UTrStatsInterface* that, UTrStatsInterface_execBaseUpgrade_Parms* params) {

}