#include "Mods.h"

namespace GameBalance {
	ValueType Property::getType() {
		return type;
	}

	bool Property::apply(PropValue value, UObject* obj) {
		return (value.type == type && applier(value, obj));
	}

	bool Property::get(UObject* obj, PropValue& ret) {
		return getter(obj, ret);
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

template <GameBalance::Items::PropId x>
static int getItemPropId() {
	return (int)x;
}

namespace LuaAPI {
	void addGameBalanceAPI(luabridge::Namespace ns) {
		ns
			.beginNamespace("Items")
				.addFunction("setProperty", &setWeaponProp)

				.beginNamespace("Properties")
					.addProperty<int, int>("Invalid", &getItemPropId<GameBalance::Items::PropId::INVALID>)
					// Ammo
					.addProperty<int, int>("ClipAmmo", &getItemPropId<GameBalance::Items::PropId::CLIP_AMMO>)
					.addProperty<int, int>("SpareAmmo", &getItemPropId<GameBalance::Items::PropId::SPARE_AMMO>)
					.addProperty<int, int>("AmmoPerShot", &getItemPropId<GameBalance::Items::PropId::AMMO_PER_SHOT>)
					.addProperty<int, int>("LowAmmoCutoff", &getItemPropId<GameBalance::Items::PropId::LOW_AMMO_CUTOFF>)
					// Reload / Firing
					.addProperty<int, int>("ReloadTime", &getItemPropId<GameBalance::Items::PropId::RELOAD_TIME>)
					.addProperty<int, int>("FireInterval", &getItemPropId<GameBalance::Items::PropId::FIRE_INTERVAL>)
					.addProperty<int, int>("HoldToFire", &getItemPropId<GameBalance::Items::PropId::HOLD_TO_FIRE>)
					.addProperty<int, int>("CanZoom", &getItemPropId<GameBalance::Items::PropId::CAN_ZOOM>)
					.addProperty<int, int>("ReloadSingle", &getItemPropId<GameBalance::Items::PropId::RELOAD_SINGLE>)
					.addProperty<int, int>("ReloadApplicationProportion", &getItemPropId<GameBalance::Items::PropId::RELOAD_APPLICATION_PROPORTION>)
					// Damage / Impact
					.addProperty<int, int>("Damage", &getItemPropId<GameBalance::Items::PropId::DAMAGE>)
					.addProperty<int, int>("ExplosiveRadius", &getItemPropId<GameBalance::Items::PropId::EXPLOSIVE_RADIUS>)
					.addProperty<int, int>("DirectHitMultiplier", &getItemPropId<GameBalance::Items::PropId::DIRECT_HIT_MULTIPLIER>)
					.addProperty<int, int>("ImpactMomentum", &getItemPropId<GameBalance::Items::PropId::IMPACT_MOMENTUM>)
					.addProperty<int, int>("SelfImpactMomentumMultiplier", &getItemPropId<GameBalance::Items::PropId::SELF_IMPACT_MOMENTUM_MULTIPLIER>)
					.addProperty<int, int>("SelfImpactExtraZMomentum", &getItemPropId<GameBalance::Items::PropId::SELF_IMPACT_EXTRA_Z_MOMENTUM>)
					// Projectile / Tracer
					.addProperty<int, int>("ProjectileSpeed", &getItemPropId<GameBalance::Items::PropId::PROJECTILE_SPEED>)
					.addProperty<int, int>("ProjectileMaxSpeed", &getItemPropId<GameBalance::Items::PropId::PROJECTILE_MAX_SPEED>)
					.addProperty<int, int>("CollisionSize", &getItemPropId<GameBalance::Items::PropId::COLLISION_SIZE>)
					.addProperty<int, int>("ProjectileInheritence", &getItemPropId<GameBalance::Items::PropId::PROJECTILE_INHERITENCE>)
					.addProperty<int, int>("ProjectileLifespan", &getItemPropId<GameBalance::Items::PropId::PROJECTILE_LIFESPAN>)
					.addProperty<int, int>("ProjectileGravity", &getItemPropId<GameBalance::Items::PropId::PROJECTILE_GRAVITY>)
					.addProperty<int, int>("ProjectileTerminalVelocity", &getItemPropId<GameBalance::Items::PropId::PROJECTILE_TERMINAL_VELOCITY>)
					.addProperty<int, int>("ProjectileBounceDamping", &getItemPropId<GameBalance::Items::PropId::PROJECTILE_BOUNCE_DAMPING>)
					.addProperty<int, int>("ProjectileMeshScale", &getItemPropId<GameBalance::Items::PropId::PROJECTILE_MESH_SCALE>)
					.addProperty<int, int>("ProjectileLightRadius", &getItemPropId<GameBalance::Items::PropId::PROJECTILE_LIGHT_RADIUS>)
					.addProperty<int, int>("HitscanRange", &getItemPropId<GameBalance::Items::PropId::HITSCAN_RANGE>)
					// Accuracy
					.addProperty<int, int>("Accuracy", &getItemPropId<GameBalance::Items::PropId::ACCURACY>)
					.addProperty<int, int>("AccuracyLossOnShot", &getItemPropId<GameBalance::Items::PropId::ACCURACY_LOSS_ON_SHOT>)
					.addProperty<int, int>("AccuracyLossOnJump", &getItemPropId<GameBalance::Items::PropId::ACCURACY_LOSS_ON_JUMP>)
					.addProperty<int, int>("AccuracyLossMax", &getItemPropId<GameBalance::Items::PropId::ACCURACY_LOSS_MAX>)
					.addProperty<int, int>("AccuracyCorrectionRate", &getItemPropId<GameBalance::Items::PropId::ACCURACY_CORRECTION_RATE>)

				.endNamespace()
			.endNamespace();
	}
}