#include "Mods.h"

using namespace GameBalance;

namespace GameBalance {
	ValueType Property::getType() {
		return type;
	}

	std::string getValueTypeName(ValueType t) {
		switch (t) {
		case ValueType::BOOLEAN:
			return "boolean";
		case ValueType::INTEGER:
			return "integer";
		case ValueType::FLOAT:
			return "float";
		case ValueType::STRING:
			return "string";
		}
		return "Invalid";
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

template <typename IdType>
static void applyPropConfig(std::map<int, UClass*>& relevantClassDefs, std::map<IdType, Property>& propDefs, UClass* requiredSuperClass, std::map<int, std::map<IdType, PropValue> >& props) {
	for (auto& elem : props) {
		auto& elem_it = relevantClassDefs.find(elem.first);
		if (elem_it == relevantClassDefs.end()) {
			Logger::error("Unable to set property; invalid id %d", elem.first);
			continue;
		}

		std::vector<UObject*> objectsToApplyOn;

		// Special case for non-items, because the generated SDK can't get the default properly from the static class
		// Have to do a search by name instead
		bool isClassCase = std::is_same<IdType, Classes::PropId>::value;
		bool isVehicleCase = std::is_same<IdType, Vehicles::PropId>::value;
		bool isVehicleWeaponCase = std::is_same<IdType, VehicleWeapons::PropId>::value;
		if (isClassCase || isVehicleCase || isVehicleWeaponCase) {
			std::string namePrefix;
			std::string name;
			if (isClassCase) {
				namePrefix = "TrFamilyInfo";
				auto& fiName = Data::class_id_to_name.find(elem.first);
				name = fiName->second;
			} else if (isVehicleCase) {
				namePrefix = "TrVehicle";
				auto& vehName = Data::vehicle_id_to_name.find(elem.first);
				name = vehName->second;
			} else if (isVehicleWeaponCase) {
				namePrefix = "TrVehicleWeapon";
				auto& wepName = Data::vehicle_weapon_id_to_name.find(elem.first);
				name = wepName->second;
			}

			Logger::error("Special case! %d", elem.first);

			// Find the default object given this name
			std::string defName = namePrefix + "_" + name + " TribesGame.Default__" + namePrefix + "_" + name;
			if (isClassCase) {
				// Need both BE and DS variants in this case
				std::string beName = namePrefix + "_" + name + "_BE" + " TribesGame.Default__" + namePrefix + "_" + name + "_BE";
				std::string dsName = namePrefix + "_" + name + "_DS" + " TribesGame.Default__" + namePrefix + "_" + name + "_DS";
				UObject* objBE = UObject::FindObject<UTrFamilyInfo>(beName.c_str());
				UObject* objDS = UObject::FindObject<UTrFamilyInfo>(dsName.c_str());
				if (objBE && objDS) {
					objectsToApplyOn.push_back(objBE);
					objectsToApplyOn.push_back(objDS);
				}
			} else if (isVehicleCase) {
				UObject* obj = UObject::FindObject<ATrVehicle>(defName.c_str());
				if (obj) objectsToApplyOn.push_back(obj);
			}
			else if (isVehicleWeaponCase) {
				UObject* obj = UObject::FindObject<ATrVehicleWeapon>(defName.c_str());
				if (obj) objectsToApplyOn.push_back(obj);
			}
			
			if (objectsToApplyOn.empty()) {
				Logger::error("Unable to set property; failed to get default object for id %d", elem.first);
				continue;
			}
		}

		if (objectsToApplyOn.empty()) {
			Logger::error("Non special case: %d, elem_it->second->Default = %p", elem.first, elem_it->second->Default);
			if (!elem_it->second || !elem_it->second->Default || !elem_it->second->Default->IsA(requiredSuperClass)) {
				Logger::error("Unable to set property; failed to get default object for id %d", elem.first);
				continue;
			}
			objectsToApplyOn.push_back(elem_it->second->Default);
		}

		for (auto& prop : elem.second) {
			auto& it = propDefs.find(prop.first);
			if (it == propDefs.end()) {
				// Non-existent property, fail
				Logger::error("Unable to set property; invalid property id %d", prop.first);
				continue;
			}

			for (auto& obj : objectsToApplyOn) {
				it->second.apply(prop.second, obj);
			}
		}
	}
}

void ServerSettings::ApplyGameBalanceProperties() {
	applyPropConfig(Data::weapon_id_to_weapon_class, Items::properties, ATrDevice::StaticClass(), weaponProperties);
	applyPropConfig(Data::class_id_to_class, Classes::properties, UTrFamilyInfo::StaticClass(), classProperties);
	applyPropConfig(Data::vehicle_id_to_class, Vehicles::properties, ATrVehicle::StaticClass(), vehicleProperties);
	applyPropConfig(Data::vehicle_weapon_id_to_class, VehicleWeapons::properties, ATrVehicleWeapon::StaticClass(), vehicleWeaponProperties);
}

static bool getPropValFromLua(ValueType expectedType, LuaRef val, PropValue& ret) {
	switch (expectedType) {
	case ValueType::BOOLEAN:
		if (val.type() != 1) {
			return false;
		}
		ret = PropValue::fromBool(val);
		break;
	case ValueType::INTEGER:
		float intPart;
		if (!val.isNumber() || modf((float)val, &intPart) != 0) {
			return false;
		}
		ret = PropValue::fromInt(val);
		break;
	case ValueType::FLOAT:
		if (!val.isNumber()) {
			return false;
		}
		ret = PropValue::fromFloat(val);
		break;
	case ValueType::STRING:
		if (!val.isString()) {
			return false;
		}
		ret = PropValue::fromString(val);
		break;
	}

	return true;
}

template <typename IdType>
static void setProp(std::map<IdType, Property>& propDefs, std::map<int, std::map<IdType, PropValue> >& props, int elemId, int intPropId, LuaRef val) {
	IdType propId = (IdType)intPropId;
	auto& it = propDefs.find(propId);
	if (it == propDefs.end()) {
		// Non-existent property, fail
		Logger::error("Unable to set property config; invalid property id %d", propId);
		return;
	}

	PropValue propVal;
	if (!getPropValFromLua(it->second.getType(), val, propVal)) {
		Logger::error("Unable to set property config for propId %d, wrong type (expected %s)",
			propId,
			getValueTypeName(it->second.getType()).c_str());
	}

	auto& cit = props.find(elemId);
	if (cit == props.end()) {
		props[elemId] = std::map<IdType, PropValue>();
	}
	props[elemId][propId] = propVal;
}

static void setWeaponProp(std::string className, std::string itemName, int intPropId, LuaRef val) {
	int itemId = Data::getItemId(className, itemName);

	setProp(Items::properties, g_config.serverSettings.weaponProperties, itemId, intPropId, val);
}

static void setClassProp(std::string className, int intPropId, LuaRef val) {
	int classNum = Utils::searchMapId(Data::classes, className, "", false);
	if (classNum == 0) {
		Logger::error("Unable to set property config; invalid class %s", className.c_str());
		return;
	}
	int classId = Data::classes_id[classNum - 1];

	setProp(Classes::properties, g_config.serverSettings.classProperties, classId, intPropId, val);
}

static void setVehicleProp(std::string vehicleName, int intPropId, LuaRef val) {
	int vehicleId = Utils::searchMapId(Data::vehicles, vehicleName, "", false);
	if (vehicleId == 0) {
		Logger::error("Unable to set property config; invalid vehicle %s", vehicleName.c_str());
		return;
	}

	setProp(Vehicles::properties, g_config.serverSettings.vehicleProperties, vehicleId, intPropId, val);
}

static void setVehicleWeaponProp(std::string vehicleWeaponName, int intPropId, LuaRef val) {
	int vehicleWeaponId = Utils::searchMapId(Data::vehicle_weapons, vehicleWeaponName, "", false);
	if (vehicleWeaponId == 0) {
		Logger::error("Unable to set property config; invalid vehicle %s", vehicleWeaponName.c_str());
		return;
	}

	setProp(VehicleWeapons::properties, g_config.serverSettings.vehicleWeaponProperties, vehicleWeaponId, intPropId, val);
}

template <typename IdType, IdType x>
static int getPropId() {
	return (int)x;
}

namespace LuaAPI {
	void addGameBalanceAPI(luabridge::Namespace ns) {
		ns
			.beginNamespace("Items")
				.addFunction("setProperty", &setWeaponProp)
				.beginNamespace("Properties")
					.addProperty<int, int>("Invalid", &getPropId<Items::PropId, Items::PropId::INVALID>)
					// Ammo
					.addProperty<int, int>("ClipAmmo", &getPropId<Items::PropId, Items::PropId::CLIP_AMMO>)
					.addProperty<int, int>("SpareAmmo", &getPropId<Items::PropId, Items::PropId::SPARE_AMMO>)
					.addProperty<int, int>("AmmoPerShot", &getPropId<Items::PropId, Items::PropId::AMMO_PER_SHOT>)
					.addProperty<int, int>("LowAmmoCutoff", &getPropId<Items::PropId, Items::PropId::LOW_AMMO_CUTOFF>)
					// Reload / Firing
					.addProperty<int, int>("ReloadTime", &getPropId<Items::PropId, Items::PropId::RELOAD_TIME>)
					.addProperty<int, int>("FireInterval", &getPropId<Items::PropId, Items::PropId::FIRE_INTERVAL>)
					.addProperty<int, int>("HoldToFire", &getPropId<Items::PropId, Items::PropId::HOLD_TO_FIRE>)
					.addProperty<int, int>("CanZoom", &getPropId<Items::PropId, Items::PropId::CAN_ZOOM>)
					.addProperty<int, int>("ReloadSingle", &getPropId<Items::PropId, Items::PropId::RELOAD_SINGLE>)
					.addProperty<int, int>("ReloadApplicationProportion", &getPropId<Items::PropId, Items::PropId::RELOAD_APPLICATION_PROPORTION>)
					// Damage / Impact
					.addProperty<int, int>("Damage", &getPropId<Items::PropId, Items::PropId::DAMAGE>)
					.addProperty<int, int>("ExplosiveRadius", &getPropId<Items::PropId, Items::PropId::EXPLOSIVE_RADIUS>)
					.addProperty<int, int>("DirectHitMultiplier", &getPropId<Items::PropId, Items::PropId::DIRECT_HIT_MULTIPLIER>)
					.addProperty<int, int>("ImpactMomentum", &getPropId<Items::PropId, Items::PropId::IMPACT_MOMENTUM>)
					.addProperty<int, int>("SelfImpactMomentumMultiplier", &getPropId<Items::PropId, Items::PropId::SELF_IMPACT_MOMENTUM_MULTIPLIER>)
					.addProperty<int, int>("SelfImpactExtraZMomentum", &getPropId<Items::PropId, Items::PropId::SELF_IMPACT_EXTRA_Z_MOMENTUM>)
					// Projectile / Tracer
					.addProperty<int, int>("ProjectileSpeed", &getPropId<Items::PropId, Items::PropId::PROJECTILE_SPEED>)
					.addProperty<int, int>("ProjectileMaxSpeed", &getPropId<Items::PropId, Items::PropId::PROJECTILE_MAX_SPEED>)
					.addProperty<int, int>("CollisionSize", &getPropId<Items::PropId, Items::PropId::COLLISION_SIZE>)
					.addProperty<int, int>("ProjectileInheritence", &getPropId<Items::PropId, Items::PropId::PROJECTILE_INHERITENCE>)
					.addProperty<int, int>("ProjectileLifespan", &getPropId<Items::PropId, Items::PropId::PROJECTILE_LIFESPAN>)
					.addProperty<int, int>("ProjectileGravity", &getPropId<Items::PropId, Items::PropId::PROJECTILE_GRAVITY>)
					.addProperty<int, int>("ProjectileTerminalVelocity", &getPropId<Items::PropId, Items::PropId::PROJECTILE_TERMINAL_VELOCITY>)
					.addProperty<int, int>("ProjectileBounceDamping", &getPropId<Items::PropId, Items::PropId::PROJECTILE_BOUNCE_DAMPING>)
					.addProperty<int, int>("ProjectileMeshScale", &getPropId<Items::PropId, Items::PropId::PROJECTILE_MESH_SCALE>)
					.addProperty<int, int>("ProjectileLightRadius", &getPropId<Items::PropId, Items::PropId::PROJECTILE_LIGHT_RADIUS>)
					.addProperty<int, int>("HitscanRange", &getPropId<Items::PropId, Items::PropId::HITSCAN_RANGE>)
					// Accuracy
					.addProperty<int, int>("Accuracy", &getPropId<Items::PropId, Items::PropId::ACCURACY>)
					.addProperty<int, int>("AccuracyLossOnShot", &getPropId<Items::PropId, Items::PropId::ACCURACY_LOSS_ON_SHOT>)
					.addProperty<int, int>("AccuracyLossOnJump", &getPropId<Items::PropId, Items::PropId::ACCURACY_LOSS_ON_JUMP>)
					.addProperty<int, int>("AccuracyLossMax", &getPropId<Items::PropId, Items::PropId::ACCURACY_LOSS_MAX>)
					.addProperty<int, int>("AccuracyCorrectionRate", &getPropId<Items::PropId, Items::PropId::ACCURACY_CORRECTION_RATE>)

				.endNamespace()
			.endNamespace()
			.beginNamespace("Classes")
				.addFunction("setProperty", &setClassProp)
				.beginNamespace("Properties")
					.addProperty<int, int>("HealthPool", &getPropId<Classes::PropId, Classes::PropId::HEALTH_POOL>)
				.endNamespace()
			.endNamespace()
			.beginNamespace("Vehicles")
				.addFunction("setProperty", &setVehicleProp)
				.beginNamespace("Properties")
					.addProperty<int, int>("HealthPool", &getPropId<Vehicles::PropId, Vehicles::PropId::HEALTH_POOL>)
				.endNamespace()
			.endNamespace()
			.beginNamespace("VehicleWeapons")
				.addFunction("setProperty", &setVehicleWeaponProp)
				.beginNamespace("Properties")
					.addProperty<int, int>("ClipAmmo", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::CLIP_AMMO>)
				.endNamespace()
			.endNamespace()
			;
	}
}