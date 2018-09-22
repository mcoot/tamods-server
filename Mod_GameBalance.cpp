#include "Mods.h"

namespace GameBalance {
	ValueType Property::getType() {
		return type;
	}

	bool Property::apply(PropValue value, UObject* obj) {
		return (value.type == type && applier(value, obj));
	}
}

namespace DCServer {
	void Server::handler_PlayerConnectionMessage(std::shared_ptr<PlayerConnection> pconn, const json& j) {
		PlayerConnectionMessage msg;
		if (!msg.fromJson(j)) {
			// Failed to parse
			Logger::warn("Failed to parse connection message from player: %s", j.dump().c_str());
			return;
		}

		if (!validatePlayerConn(pconn, msg)) {
			Logger::warn("Failed to validate connection message from player: %s", j.dump().c_str());
			return;
		}

		// Send the player the current balance state
		sendGameBalanceDetailsMessage(pconn, g_config.serverSettings.weaponProperties);
	}
}



void ServerSettings::ApplyWeaponProperties() {
	for (auto& item : weaponProperties) {
		auto& wepit = Data::weapon_id_to_weapon_class.find(item.first);
		if (wepit == Data::weapon_id_to_weapon_class.end()) {
			Logger::error("Unable to set weapon property; invalid weapon id %d", item.first);
			continue;
		}

		if (!wepit->second || !wepit->second->Default || !wepit->second->Default->IsA(ATrDevice::StaticClass())) {
			Logger::error("Unable to set weapon property; failed to get default weapon for id %d", item.first);
			continue;
		}

		for (auto& prop : item.second) {
			auto& it = GameBalance::Items::properties.find(prop.first);
			if (it == GameBalance::Items::properties.end()) {
				// Non-existent property, fail
				Logger::error("Unable to set weapon property; invalid property id %d", prop.first);
				continue;
			}

			it->second.apply(prop.second, (ATrDevice*)wepit->second->Default);
		}
	}
}

static void setWeaponProp(std::string className, std::string itemName, int intPropId, LuaRef val) {
	int itemId = Data::getItemId(className, itemName);

	GameBalance::Items::PropId propId = (GameBalance::Items::PropId)intPropId;
	auto& it = GameBalance::Items::properties.find(propId);
	if (it == GameBalance::Items::properties.end()) {
		// Non-existent property, fail
		Logger::error("Unable to set weapon property config; invalid property id %d", propId);
		return;
	}

	GameBalance::PropValue propVal;

	switch (it->second.getType()) {
	case GameBalance::ValueType::BOOLEAN:
		if (val.type() != 1) {
			Logger::error("Unable to set weapon property config for propId %d, wrong type (should be boolean)", propId);
			return;
		}
		propVal = GameBalance::PropValue::fromBool(val);
		break;
	case GameBalance::ValueType::INTEGER:
		float intPart;
		if (!val.isNumber() || modf((float)val, &intPart) != 0) {
			Logger::error("Unable to set weapon property config for propId %d, wrong type (should be integer)", propId);
			return;
		}
		propVal = GameBalance::PropValue::fromInt(val);
		break;
	case GameBalance::ValueType::FLOAT:
		if (!val.isNumber()) {
			Logger::error("Unable to set weapon property config for propId %d, wrong type (should be float)", propId);
			return;
		}
		propVal = GameBalance::PropValue::fromFloat(val);
		break;
	case GameBalance::ValueType::STRING:
		if (!val.isString()) {
			Logger::error("Unable to set weapon property config for propId %d, wrong type (should be string)", propId);
			return;
		}
		propVal = GameBalance::PropValue::fromString(val);
		break;
	}

	auto& cit = g_config.serverSettings.weaponProperties.find(itemId);
	if (cit == g_config.serverSettings.weaponProperties.end()) {
		g_config.serverSettings.weaponProperties[itemId] = GameBalance::Items::PropMapping();
	}
	g_config.serverSettings.weaponProperties[itemId][propId] = propVal;
}

static int PropId_Invalid = (int)GameBalance::Items::PropId::INVALID;
static int PropId_ClipAmmo = (int)GameBalance::Items::PropId::CLIP_AMMO;
static int PropId_SpareAmmo = (int)GameBalance::Items::PropId::SPARE_AMMO;
static int PropId_AmmoPerShot = (int)GameBalance::Items::PropId::AMMO_PER_SHOT;
static int PropId_LowAmmoCutoff = (int)GameBalance::Items::PropId::LOW_AMMO_CUTOFF;

namespace LuaAPI {
	void addGameBalanceAPI(luabridge::Namespace ns) {
		ns
			.beginNamespace("Items")
				.addFunction("setProperty", &setWeaponProp)

				.beginNamespace("Properties")
					.addVariable("Invalid", &PropId_Invalid, false)
					.addVariable("ClipAmmo", &PropId_ClipAmmo, false)
					.addVariable("SpareAmmo", &PropId_SpareAmmo, false)
					.addVariable("AmmoPerShot", &PropId_AmmoPerShot, false)
					.addVariable("LowAmmoCutoff", &PropId_LowAmmoCutoff, false)
				.endNamespace()
			.endNamespace();
	}
}