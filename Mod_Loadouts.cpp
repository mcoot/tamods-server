#include "Mods.h"
#include "LoadoutTypes.h"

void TrDevice_GetReloadTime(ATrDevice* that, ATrDevice_execGetReloadTime_Parms* params, float* result, Hooks::CallInfo* callInfo) {
	Logger::debug("[GetReloadTime] Caller function: %s::%s", callInfo->callerObject->Name.GetName(), callInfo->caller->Name.GetName());
	float r = that->GetReloadTime(params->PRI, params->equipPoint);
	Logger::debug("[GetReloadTime] Reload time for %s = %f", that->Name.GetName(), r);
	*result = r;	
}

static bool isWeaponEquipPoint(int eqp) {
	return eqp == EQP_Primary || eqp == EQP_Secondary || eqp == EQP_Tertiary || eqp == EQP_Quaternary || eqp == EQP_Belt;
}


bool CustomClass::doesLoadoutMatch(int ootbClass, const std::map<int, int>& loadout) {
	if (ootbClass != this->ootbClass) return false;

	for (auto& eqp : equipPointsToValidate) {
		// Skip missing equip points from the loadout
		auto& it = loadout.find(eqp);
		if (it == loadout.end()) continue;

		int itemId = it->second;
		if (itemId != 0 && !this->allowedItems.count(itemId)) {
			//Logger::debug("Tried to check loadout against class %d, but failed on item %d", this->armorClass, itemId);
			return false;
		}
	}


	return true;
}

static void applyHardcodedLoadout(ATrPlayerReplicationInfo* that, int classId, int slot) {
	auto& it = Data::class_id_to_nb.find(classId);
	if (it == Data::class_id_to_nb.end()) return;
	unsigned classNum = it->second;
	for (int i = 0; i < EQP_MAX; ++i) {
		std::string item = g_config.hardcodedLoadouts.get(classNum, slot, i);
		if (item != "") {
			int itemId = 0;
			itemId = Data::getItemId(Data::class_type_name[classNum], item);
			if (itemId != 0) {
				that->r_EquipLevels[i].EquipId = itemId;
			}
		}
	}
}

static void applyTAServerLoadout(ATrPlayerReplicationInfo* that, int classId, int slot) {
	if (!g_TAServerClient.isConnected()) return;

	std::map<int, int> taserverRetrievedMap;
	if (!g_TAServerClient.retrieveLoadout(that->UniqueId, classId, slot, taserverRetrievedMap)) {
		Logger::warn("Failed to retrieve loadout on class %d, slot %d for UniqueID %d from TAServer.", classId, slot, that->UniqueId);
	}

	Logger::debug("Retrieved loadout for class %d!", classId);

	// Apply the equipment
	for (auto& eqpItem : taserverRetrievedMap) {
		if (eqpItem.second != 0) {
			if (eqpItem.first == EQP_Tertiary) {
				// With a GOTY login server, the tertiary weapon is used to encode perks
				// See if attempting to decode makes any sense
				int decodedPerkA = Utils::perks_DecodeA(eqpItem.second);
				int decodedPerkB = Utils::perks_DecodeB(eqpItem.second);
				if (Data::perk_id_to_name.find(decodedPerkA) != Data::perk_id_to_name.end()
					&& Data::perk_id_to_name.find(decodedPerkB) != Data::perk_id_to_name.end()) {
					// Successfully decoded into two perks
					that->r_EquipLevels[EQP_Tertiary].EquipId = 0;
					that->r_EquipLevels[EQP_PerkA].EquipId = decodedPerkA;
					that->r_EquipLevels[EQP_PerkB].EquipId = decodedPerkB;
				}
				else {
					// Decoding made no sense, assume this is just a weapon
					that->r_EquipLevels[eqpItem.first].EquipId = eqpItem.second;
				}
			}
			else {
				that->r_EquipLevels[eqpItem.first].EquipId = eqpItem.second;
				if (eqpItem.first == EQP_Skin) {
					Logger::debug("Applied skin: %d", eqpItem.second);
				}
			}
		}
	}
}

static void applyWeaponBans(ATrPlayerReplicationInfo* that, int classId) {
	for (int i = 0; i < EQP_MAX; ++i) {
		// Check the player's class to see if that class is not allowed to have that slot
		bool shouldBanSlot = false;
		switch (classId) {
		case CONST_CLASS_TYPE_LIGHT_PATHFINDER:
		case CONST_CLASS_TYPE_LIGHT_SENTINEL:
		case CONST_CLASS_TYPE_LIGHT_INFILTRATOR:
			shouldBanSlot = g_config.serverSettings.disabledEquipPointsLight.count(i) != 0;
			break;
		case CONST_CLASS_TYPE_MEDIUM_SOLDIER:
		case CONST_CLASS_TYPE_MEDIUM_RAIDER:
		case CONST_CLASS_TYPE_MEDIUM_TECHNICIAN:
			shouldBanSlot = g_config.serverSettings.disabledEquipPointsMedium.count(i) != 0;
			break;
		case CONST_CLASS_TYPE_HEAVY_JUGGERNAUGHT:
		case CONST_CLASS_TYPE_HEAVY_DOOMBRINGER:
		case CONST_CLASS_TYPE_HEAVY_BRUTE:
			shouldBanSlot = g_config.serverSettings.disabledEquipPointsHeavy.count(i) != 0;
			break;
		}
		int itemId = that->r_EquipLevels[i].EquipId;

		bool shouldBanMutualExclusion = false;
		for (auto& mutualExclusion : g_config.serverSettings.mutuallyExclusiveItems) {
			if (mutualExclusion.first != itemId && mutualExclusion.second != itemId) continue;
			int exclusiveItemId = mutualExclusion.first == itemId ? mutualExclusion.second : mutualExclusion.first;
			for (int j = 0; j < EQP_MAX; ++j) {
				if (i != j && that->r_EquipLevels[j].EquipId == exclusiveItemId) {
					shouldBanMutualExclusion = true;
					break;
				}
			}
			if (shouldBanMutualExclusion) break;
		}

		// If the given weapon is banned or the slot is disabled, set its equipid to 0
		if (shouldBanSlot || shouldBanMutualExclusion
			|| g_config.serverSettings.bannedItems.count(itemId) != 0) {
			Logger::debug("Banned item %d on slot %d", that->r_EquipLevels[i].EquipId, i);
			that->r_EquipLevels[i].EquipId = 0;
		}
	}
}

void TrPlayerReplicationInfo_VerifyCharacter(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execVerifyCharacter_Parms* params, bool* result) {
	*result = true;
}

void TrPlayerController_GetFamilyInfoFromId(ATrPlayerController* that, ATrPlayerController_execGetFamilyInfoFromId_Parms* params, UClass** result) {
	if (Data::class_id_to_class.find(params->ClassId) == Data::class_id_to_class.end()) {
		*result = NULL;
	}
	else {
		*result = Data::class_id_to_class[params->ClassId];
	}
}

void TrPlayerReplicationInfo_GetCharacterEquip(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execGetCharacterEquip_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	// Apply any server hardcoded loadout configuration
	applyHardcodedLoadout(that, params->ClassId, params->Loadout);
	// If in a mode to retrieve loadout info from a server, do so
	if (g_config.connectToTAServer) {
		applyTAServerLoadout(that, params->ClassId, params->Loadout);
	}
	if (g_config.serverSettings.ForceHardcodedLoadouts) {
		applyHardcodedLoadout(that, params->ClassId, params->Loadout);
	}
	applyWeaponBans(that, params->ClassId);
}

void TrPlayerReplicationInfo_ResolveDefaultEquip(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execResolveDefaultEquip_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	// Ensure weapon bans on default weapons take place clientside by removing the defaults
	UTrFamilyInfo* familyInfo = (UTrFamilyInfo*)params->FamilyInfo->Default;
	FDeviceSelectionList noWeapon;
	noWeapon.ContentDeviceClassString = FString(L"");
	noWeapon.DeviceClass = NULL;
	std::vector<TR_EQUIP_POINT> eqpsToRemove = { EQP_Primary, EQP_Secondary, EQP_Tertiary, EQP_Quaternary, EQP_Pack, EQP_Belt, EQP_Skin, EQP_Voice };
	for (int i = 0; i < familyInfo->DevSelectionList.Count; ++i) {
		FDeviceSelectionList cur = familyInfo->DevSelectionList.GetStd(i);
		if (!cur.DeviceClass) continue;
		bool found = false;
		for (int j = 0; j < eqpsToRemove.size(); ++j) {
			if (cur.equipPoint == eqpsToRemove[j]) {
				found = true;
				break;
			}
		}
		if (!found) continue;
		noWeapon.equipPoint = cur.equipPoint;
		familyInfo->DevSelectionList.Set(i, noWeapon);
	}
}

void TAServer::Client::handler_Launcher2GameLoadoutMessage(const json& msgBody) {
	// Parse the message
	Launcher2GameLoadoutMessage msg;
	if (!msg.fromJson(msgBody)) {
		// Failed to parse
		Logger::error("Failed to parse loadout response: %s", msgBody.dump().c_str());
	}

	// Put it in the map
	std::lock_guard<std::mutex> lock(receivedLoadoutsMutex);
	receivedLoadouts[Utils::netIdToLong(msg.uniquePlayerId)] = msg;
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
