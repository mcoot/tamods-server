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

	std::vector<int> eqpPointsToVerify = {
		EQP_Primary, 
		EQP_Secondary, 
		EQP_Tertiary, 
		EQP_Quaternary, 
		EQP_Pack, 
		EQP_Belt, 
		//EQP_PerkA,
		//EQP_PerkB,
		EQP_Skin
	};

	for (auto& eqp : eqpPointsToVerify) {
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

	// If in Custom Class mode, validate that this loadout matches a custom class, and apply the custom armour class if needed
	if (g_config.serverSettings.useCustomClasses) {
		bool found = false;
		for (auto& it : g_config.serverSettings.customClasses) {
			if (it.second.doesLoadoutMatch(classId, taserverRetrievedMap)) {
				found = true;
				// Apply the custom armour class if one is specified
				if (it.second.armorClass != 0) {
					that->r_EquipLevels[EQP_Armor].EquipId = it.second.armorClass;
					Logger::debug("Changed armor class to custom one: %d", it.second.armorClass);
				}
				break;
			}
		}
		if (!found) {
			// Failed to validate
			return;
		}
	}

	// Apply the equipment
	for (auto& eqpItem : taserverRetrievedMap) {
		if (eqpItem.second != 0) {
			if (eqpItem.first == EQP_Voice) {
				// Voice field could be used to encode perks as well...
				// See if attempting to decode makes any sense
				int decodedVoice = Utils::perksAndVoice_DecodeVoice(eqpItem.second);
				int decodedPerkA = Utils::perksAndVoice_DecodePerkA(eqpItem.second);
				int decodedPerkB = Utils::perksAndVoice_DecodePerkB(eqpItem.second);
				if (
					   Data::voice_id_to_name.find(decodedVoice) != Data::voice_id_to_name.end()
					&& Data::perk_id_to_name.find(decodedPerkA) != Data::perk_id_to_name.end()
					&& Data::perk_id_to_name.find(decodedPerkB) != Data::perk_id_to_name.end()
					) {
					// Successfully decoded into a voice and two perks
					that->r_EquipLevels[EQP_Voice].EquipId = decodedVoice;
					that->r_EquipLevels[EQP_PerkA].EquipId = decodedPerkA;
					that->r_EquipLevels[EQP_PerkB].EquipId = decodedPerkB;
				}
				else {
					// Decoding made no sense, assume this is just a voice
					that->r_EquipLevels[eqpItem.first].EquipId = eqpItem.second;
				}
			}
			else {
				that->r_EquipLevels[eqpItem.first].EquipId = eqpItem.second;
			}
		}
	}
}

static void applyWeaponBans(ATrPlayerReplicationInfo* that, int classId) {
	for (int i = 0; i < EQP_MAX; ++i) {
		// Check the player's class to see if that class is not allowed to have that slot
		bool shouldBanSlot = false;
		switch (classId) {
		case CONST_CLASS_TYPE_LIGHT:
			shouldBanSlot = g_config.serverSettings.disabledEquipPointsLight.count(i) != 0;
			break;
		case CONST_CLASS_TYPE_MEDIUM:
			shouldBanSlot = g_config.serverSettings.disabledEquipPointsMedium.count(i) != 0;
			break;
		case CONST_CLASS_TYPE_HEAVY:
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
			that->r_EquipLevels[i].EquipId = 0;
		}
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
	Logger::debug("[Done GetCharacterEquip]");
}

void TrPlayerReplicationInfo_ResolveDefaultEquip(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execResolveDefaultEquip_Parms* params, void* result, Hooks::CallInfo* callInfo) {
	// Ensure weapon bans on default weapons take place clientside by removing the defaults
	UTrFamilyInfo* familyInfo = (UTrFamilyInfo*)params->FamilyInfo->Default;
	FDeviceSelectionList noWeapon;
	noWeapon.ContentDeviceClassString = FString(L"");
	noWeapon.DeviceClass = NULL;
	TR_EQUIP_POINT eqpsToRemove[8] = { EQP_Primary, EQP_Secondary, EQP_Tertiary, EQP_Quaternary, EQP_Pack, EQP_Belt, EQP_Skin, EQP_Voice };
	for (int i = 0; i < familyInfo->DevSelectionList.Count; ++i) {
		if (!familyInfo->DevSelectionList.GetStd(i).DeviceClass) continue;
		bool found = false;
		for (int j = 0; j < 8; ++j) {
			if (familyInfo->DevSelectionList.GetStd(i).equipPoint == eqpsToRemove[j]) {
				found = true;
				break;
			}
		}
		if (!found) continue;
		noWeapon.equipPoint = familyInfo->DevSelectionList.GetStd(i).equipPoint;
		familyInfo->DevSelectionList.Set(i, noWeapon);
	}
}

static void applyCustomClassToPRI(ATrPlayerReplicationInfo* that) {
	if (that->m_nPlayerClassId == that->r_EquipLevels[EQP_Armor].EquipId) {
		// We don't need to set a custom class
		return;
	}

	// Find the armour class name
	auto& it = Data::armor_class_id_to_name.find(that->r_EquipLevels[EQP_Armor].EquipId);
	if (it == Data::armor_class_id_to_name.end()) {
		Logger::warn("Loadout attempted to use invalid armor class %d", that->r_EquipLevels[EQP_Armor].EquipId);
		return;
	}

	std::string baseClassName = "Class TribesGame.TrFamilyInfo_" + it->second;
	std::string teamAppendPart = that->GetTeamNum() == 1 ? "_DS" : "_BE";
	std::string className = "Class TribesGame.TrFamilyInfo_" + it->second + teamAppendPart;
		

	// Get base armor
	//UTrFamilyInfo* baseFI = UObject::FindObject<UTrFamilyInfo>(baseClassName.c_str());
	UClass* baseFIClass = UObject::FindClass(baseClassName.c_str());
	if (!baseFIClass || !baseFIClass->Default) {
		Logger::warn("Failed to retrieve base armour class \"%s\"", baseClassName.c_str());
		return;
	}
	UTrFamilyInfo* baseFI = (UTrFamilyInfo*)baseFIClass->Default;
	// Get team-specific armor
	//UTrFamilyInfo* mainFI = UObject::FindObject<UTrFamilyInfo>(className.c_str());
	UClass* mainFIClass = UObject::FindClass(className.c_str());
	if (!mainFIClass || !mainFIClass->Default) {
		Logger::warn("Failed to retrieve specific armour class \"%s\"", className.c_str());
		return;
	}
	UTrFamilyInfo* mainFI = (UTrFamilyInfo*)mainFIClass->Default;

	that->m_CurrentBaseClass = baseFIClass;
	that->CharClassInfo = mainFIClass;

	if (that->ClassIsChildOf(that->CharClassInfo, UTrFamilyInfo_Light::StaticClass())) {
		that->m_ArmorType = ARMOR_Light;
	}
	else if (that->ClassIsChildOf(that->CharClassInfo, UTrFamilyInfo_Heavy::StaticClass())) {
		that->m_ArmorType = ARMOR_Heavy;
	}
	else {
		that->m_ArmorType = ARMOR_Medium;
	}

	that->m_nPlayerClassId = mainFI->ClassId;
	that->m_nPlayerIconIndex = 140;

	ATrPlayerController* thatPC = (ATrPlayerController*)that->Owner;

	that->r_bSkinId = that->r_EquipLevels[EQP_Skin].EquipId;

	if (that->Stats) that->Stats->SetActiveClass(thatPC, that->m_nPlayerClassId);

	if (thatPC) {
		ATrPawn* pawn = (ATrPawn*)thatPC->Pawn;

		if (pawn) {
			Logger::debug("Setting pawn with main class = \"%s\"", that->CharClassInfo->GetFullName());
			pawn->SetCharacterClassFromInfo(that->CharClassInfo, true);
			pawn->SetPending3PSkin(that->GetCurrentSkinClass(that->CharClassInfo));
		}
	}

	that->bNetDirty = true;
}

void TrPlayerReplicationInfo_GetCurrentVoiceClass(ATrPlayerReplicationInfo* that, ATrPlayerReplicationInfo_execGetCurrentVoiceClass_Parms* params, UClass** result, Hooks::CallInfo* callInfo) {
	// Get the real voice
	*result = that->InvHelper->GetEquipClass(that->r_EquipLevels[EQP_Voice].EquipId);
	if (!*result) {
		UTrFamilyInfo* fi = (UTrFamilyInfo*)params->FamilyInfo->Default;
		*result = fi->DevSelectionList.GetStd(EQP_Voice).DeviceClass;
	}

	// Apply the custom class if appropriate
	applyCustomClassToPRI(that);
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
	receivedLoadouts[netIdToLong(msg.uniquePlayerId)] = msg;
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
