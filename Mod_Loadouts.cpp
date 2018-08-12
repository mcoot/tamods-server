#include "Mods.h"
#include "LoadoutTypes.h"

static bool isWeaponEquipPoint(int eqp) {
	return eqp == EQP_Primary || eqp == EQP_Secondary || eqp == EQP_Tertiary || eqp == EQP_Quaternary || eqp == EQP_Belt;
}


static void applyHardcodedLoadout(ATrPlayerReplicationInfo* that, int classId, int slot) {
	auto& it = Data::class_id_to_nb.find(classId);
	if (it == Data::class_id_to_nb.end()) return;
	unsigned classNum = it->second;
	for (int i = 0; i < EQP_MAX; ++i) {
		std::string item = g_config.hardcodedLoadouts.get(classNum, slot, i);
		if (item != "") {
			int itemId = 0;
			if (isWeaponEquipPoint(i)) {
				itemId = Utils::searchMapId(Data::weapons[classNum], item, "", false);
			}
			else if (i == EQP_Pack) {
				itemId = Utils::searchMapId(Data::packs[classNum], item, "", false);
			}
			else if (i == EQP_Skin) {
				itemId = Utils::searchMapId(Data::skins[classNum], item, "", false);
			}
			else if (i == EQP_Voice) {
				itemId = Utils::searchMapId(Data::voices, item, "", false);
			}
			if (itemId != 0) {
				that->r_EquipLevels[i].EquipId = itemId;
			}
		}
	}
	
}

void TrPlayerReplicationInfo_GetCharacterEquip(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execGetCharacterEquip_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	// Apply any server hardcoded loadout configuration
	applyHardcodedLoadout(that, params->ClassId, params->Loadout);
	// If in a mode to retrieve loadout info from a server, do so
	switch (g_config.serverMode) {
	case ServerMode::TASERVER:
		// TBA
		break;
	case ServerMode::API:
		// TBA
		break;
	}
}

// static variables for lua enum
static int eqpNone = EQP_NONE;
static int eqpMelee = EQP_Melee;
static int eqpPrimary = EQP_Primary;
static int eqpSecondary = EQP_Secondary;
static int eqpTertiary = EQP_Tertiary;
static int eqpQuarternary = EQP_Quaternary;
static int eqpPack = EQP_Pack;
static int eqpBelt = EQP_Belt;
static int eqpDeployable = EQP_Deployable;
static int eqpLaserTarget = EQP_LaserTarget;
static int eqpArmor = EQP_Armor;
static int eqpPerkA = EQP_PerkA;
static int eqpPerkB = EQP_PerkB;
static int eqpSkin = EQP_Skin;
static int eqpVoice = EQP_Voice;

 template <unsigned classNum>
 static std::string getEquip(int slot, int equip) {
	 return g_config.hardcodedLoadouts.getTemplated<classNum>(slot, equip);
}

 template <unsigned classNum>
 static void setEquip(int slot, int equip, std::string item) {
	 g_config.hardcodedLoadouts.setTemplated<classNum>(slot, equip, item);
}

namespace LuaAPI {
	void addLoadoutsLuaAPI(luabridge::Namespace ns) {
		ns
			.beginNamespace("Loadouts")
				.beginNamespace("EquipPoints")
					.addVariable("None", &eqpNone, false)
					.addVariable("Melee", &eqpMelee, false)
					.addVariable("Primary", &eqpPrimary, false)
					.addVariable("Secondary", &eqpSecondary, false)
					.addVariable("Tertiary", &eqpTertiary, false)
					.addVariable("Quarternary", &eqpQuarternary, false)
					.addVariable("Pack", &eqpPack, false)
					.addVariable("Belt", &eqpBelt, false)
					.addVariable("Deployable", &eqpDeployable, false)
					.addVariable("LaserTarget", &eqpLaserTarget, false)
					.addVariable("Armor", &eqpArmor, false)
					.addVariable("PerkA", &eqpPerkA, false)
					.addVariable("PerkB", &eqpPerkB, false)
					.addVariable("Skin", &eqpSkin, false)
					.addVariable("Voice", &eqpVoice, false)
				.endNamespace()

				.beginNamespace("Hardcoded")
					.beginNamespace("Light")
						.addFunction("get", &getEquip<0>)
						.addFunction("set", &setEquip<0>)
					.endNamespace()
					.beginNamespace("Medium")
						.addFunction("get", &getEquip<1>)
						.addFunction("set", &setEquip<1>)
					.endNamespace()
					.beginNamespace("Heavy")
						.addFunction("get", &getEquip<2>)
						.addFunction("set", &setEquip<2>)
					.endNamespace()
				.endNamespace()
			.endNamespace()
			;
	}
}
