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
		bool isMeleeCase = std::is_same<IdType, Items::PropId>::value && elem.first == CONST_WEAPON_ID_MELEE; // Melee is special cased because it has BE and DS variants
		if (isClassCase || isVehicleCase || isVehicleWeaponCase || isMeleeCase) {
			std::string namePrefix;
			std::string name;
			if (isClassCase) {
				namePrefix = "TrFamilyInfo";
				auto& fiName = Data::class_id_to_name.find(elem.first);
				name = fiName->second;
			}
			else if (isVehicleCase) {
				namePrefix = "TrVehicle";
				auto& vehName = Data::vehicle_id_to_name.find(elem.first);
				name = vehName->second;
			}
			else if (isVehicleWeaponCase) {
				namePrefix = "TrVehicleWeapon";
				auto& wepName = Data::vehicle_weapon_id_to_name.find(elem.first);
				name = wepName->second;
			}
			else if (isMeleeCase) {
				namePrefix = "TrDevice";
				auto& meleeName = Data::weapon_id_to_name.find(elem.first);
				name = meleeName->second;
			}

			// Find the default object given this name
			std::string defName = namePrefix + "_" + name + " TribesGame.Default__" + namePrefix + "_" + name;
			if (isClassCase || isMeleeCase) {
				// Need both BE and DS variants in this case
				std::string beName = namePrefix + "_" + name + "_BE" + " TribesGame.Default__" + namePrefix + "_" + name + "_BE";
				std::string dsName = namePrefix + "_" + name + "_DS" + " TribesGame.Default__" + namePrefix + "_" + name + "_DS";
				UObject* objNormal = NULL;
				UObject* objBE = NULL;
				UObject* objDS = NULL;
				if (isClassCase) {
					objBE = UObject::FindObject<UTrFamilyInfo>(beName.c_str());
					objDS = UObject::FindObject<UTrFamilyInfo>(dsName.c_str());
				}
				else if (isMeleeCase) {
					objBE = UObject::FindObject<ATrDevice>(beName.c_str());
					objDS = UObject::FindObject<ATrDevice>(dsName.c_str());
					objNormal = UObject::FindObject<ATrDevice>(defName.c_str());
				}
				
				if (objBE && objDS) {
					objectsToApplyOn.push_back(objBE);
					objectsToApplyOn.push_back(objDS);
				}
				if (objNormal) {
					objectsToApplyOn.push_back(objNormal);
				}
			}
			else if (isVehicleCase) {
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
				if (!it->second.apply(prop.second, obj)) {
					Logger::error("Setting property id %d failed on %d", prop.first, elem.first);
				}
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

template <typename IdType>
static LuaRef getProp(std::map<int, UClass*>& relevantClassDefs, std::map<IdType, Property>& propDefs, UClass* requiredSuperClass, int elemId, int intPropId) {
	auto& elem_it = relevantClassDefs.find(elemId);
	if (elem_it == relevantClassDefs.end()) {
		Logger::error("Unable to get property; invalid id %d", elemId);
		// Return nil Lua value
		return LuaRef(g_config.lua.getState());
	}

	if (!elem_it->second || !elem_it->second->Default || !elem_it->second->Default->IsA(requiredSuperClass)) {
		Logger::error("Unable to get property; failed to get default object for id %d", elemId);
		return LuaRef(g_config.lua.getState());
	}

	auto& it = propDefs.find((IdType)intPropId);
	if (it == propDefs.end()) {
		// Non-existent property, fail
		Logger::error("Unable to get property; invalid property id %d", intPropId);
		return LuaRef(g_config.lua.getState());
	}
	GameBalance::PropValue val;
	if (!it->second.get(elem_it->second->Default, val)) {
		// Failed get
		Logger::error("Unable to get property with id %d", intPropId);
		return LuaRef(g_config.lua.getState());
	}

	return val.getAsLuaRef(g_config.lua.getState());
}

static LuaRef getWeaponProp(std::string className, std::string itemName, int intPropId) {
	int itemId = Data::getItemId(className, itemName);

	return getProp(Data::weapon_id_to_weapon_class, Items::properties, ATrDevice::StaticClass(), itemId, intPropId);
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

template <const char* s>
static std::string getStringVal() {
	return std::string(s);
}

namespace LuaAPI {
	void addGameBalanceAPI(luabridge::Namespace ns) {
		ns
			.beginNamespace("Items")
				.addFunction("setProperty", &setWeaponProp)
				.addFunction("getProperty", &getWeaponProp)
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
					.addProperty<int, int>("EnergyDrain", &getPropId<Items::PropId, Items::PropId::ENERGY_DRAIN>)
					.addProperty<int, int>("MaxDamageRangeProportion", &getPropId<Items::PropId, Items::PropId::MAX_DAMAGE_RANGE_PROPORTION>)
					.addProperty<int, int>("MinDamageRangeProportion", &getPropId<Items::PropId, Items::PropId::MIN_DAMAGE_RANGE_PROPORTION>)
					.addProperty<int, int>("MinDamageProportion", &getPropId<Items::PropId, Items::PropId::MIN_DAMAGE_PROPORTION>)
					.addProperty<int, int>("BulletDamageRange", &getPropId<Items::PropId, Items::PropId::BULLET_DAMAGE_RANGE>)
					.addProperty<int, int>("DamageAgainstArmorMultiplier", &getPropId<Items::PropId, Items::PropId::DAMAGE_AGAINST_ARMOR_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstGeneratorMultiplier", &getPropId<Items::PropId, Items::PropId::DAMAGE_AGAINST_GENERATOR_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstBaseTurretMultiplier", &getPropId<Items::PropId, Items::PropId::DAMAGE_AGAINST_BASE_TURRET_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstBaseSensorMultiplier", &getPropId<Items::PropId, Items::PropId::DAMAGE_AGAINST_BASE_SENSOR_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstGravCycleMultiplier", &getPropId<Items::PropId, Items::PropId::DAMAGE_AGAINST_GRAVCYCLE_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstBeowulfMultiplier", &getPropId<Items::PropId, Items::PropId::DAMAGE_AGAINST_BEOWULF_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstShrikeMultiplier", &getPropId<Items::PropId, Items::PropId::DAMAGE_AGAINST_SHRIKE_MULTIPLIER>)
					.addProperty<int, int>("DoesGibOnKill", &getPropId<Items::PropId, Items::PropId::DOES_GIB_ON_KILL>)
					.addProperty<int, int>("GibImpulseRadius", &getPropId<Items::PropId, Items::PropId::GIB_IMPULSE_RADIUS>)
					.addProperty<int, int>("GibStrength", &getPropId<Items::PropId, Items::PropId::GIB_STRENGTH>)
					.addProperty<int, int>("DoesImpulseFlag", &getPropId<Items::PropId, Items::PropId::DOES_IMPULSE_FLAG>)
					.addProperty<int, int>("MeleeDamageRadius", &getPropId<Items::PropId, Items::PropId::MELEE_DAMAGE_RADIUS>)
					.addProperty<int, int>("MeleeConeAngle", &getPropId<Items::PropId, Items::PropId::MELEE_CONE_ANGLE>)
					// Projectile / Tracer
					.addProperty<int, int>("ProjectileSpeed", &getPropId<Items::PropId, Items::PropId::PROJECTILE_SPEED>)
					.addProperty<int, int>("ProjectileMaxSpeed", &getPropId<Items::PropId, Items::PropId::PROJECTILE_MAX_SPEED>)
					.addProperty<int, int>("CollisionSize", &getPropId<Items::PropId, Items::PropId::COLLISION_SIZE>)
					.addProperty<int, int>("ProjectileInheritance", &getPropId<Items::PropId, Items::PropId::PROJECTILE_INHERITANCE>)
					.addProperty<int, int>("ProjectileLifespan", &getPropId<Items::PropId, Items::PropId::PROJECTILE_LIFESPAN>)
					.addProperty<int, int>("ProjectileGravity", &getPropId<Items::PropId, Items::PropId::PROJECTILE_GRAVITY>)
					.addProperty<int, int>("ProjectileTerminalVelocity", &getPropId<Items::PropId, Items::PropId::PROJECTILE_TERMINAL_VELOCITY>)
					.addProperty<int, int>("ProjectileBounceDamping", &getPropId<Items::PropId, Items::PropId::PROJECTILE_BOUNCE_DAMPING>)
					.addProperty<int, int>("ProjectileMeshScale", &getPropId<Items::PropId, Items::PropId::PROJECTILE_MESH_SCALE>)
					.addProperty<int, int>("ProjectileLightRadius", &getPropId<Items::PropId, Items::PropId::PROJECTILE_LIGHT_RADIUS>)
					.addProperty<int, int>("HitscanRange", &getPropId<Items::PropId, Items::PropId::HITSCAN_RANGE>)
					.addProperty<int, int>("FireOffsetX", &getPropId<Items::PropId, Items::PropId::FIRE_OFFSET_X>)
					.addProperty<int, int>("FireOffsetY", &getPropId<Items::PropId, Items::PropId::FIRE_OFFSET_Y>)
					.addProperty<int, int>("FireOffsetZ", &getPropId<Items::PropId, Items::PropId::FIRE_OFFSET_Z>)
					// Accuracy
					.addProperty<int, int>("Accuracy", &getPropId<Items::PropId, Items::PropId::ACCURACY>)
					.addProperty<int, int>("AccuracyLossOnShot", &getPropId<Items::PropId, Items::PropId::ACCURACY_LOSS_ON_SHOT>)
					.addProperty<int, int>("AccuracyLossOnJump", &getPropId<Items::PropId, Items::PropId::ACCURACY_LOSS_ON_JUMP>)
					.addProperty<int, int>("AccuracyLossMax", &getPropId<Items::PropId, Items::PropId::ACCURACY_LOSS_MAX>)
					.addProperty<int, int>("AccuracyCorrectionRate", &getPropId<Items::PropId, Items::PropId::ACCURACY_CORRECTION_RATE>)
					// Grenade
					.addProperty<int, int>("ThrowDelay", &getPropId<Items::PropId, Items::PropId::THROW_DELAY>)
					.addProperty<int, int>("ThrowPullPinTime", &getPropId<Items::PropId, Items::PropId::THROW_PULL_PIN_TIME>)
					// Pack
					.addProperty<int, int>("PackSustainedEnergyCost", &getPropId<Items::PropId, Items::PropId::PACK_SUSTAINED_ENERGY_COST>)
					.addProperty<int, int>("ThrustPackEnergyCost", &getPropId<Items::PropId, Items::PropId::THRUST_PACK_ENERGY_COST>)
					.addProperty<int, int>("ThrustPackImpulse", &getPropId<Items::PropId, Items::PropId::THRUST_PACK_IMPULSE>)
					.addProperty<int, int>("ThrustPackSidewaysImpulse", &getPropId<Items::PropId, Items::PropId::THRUST_PACK_SIDEWAYS_IMPULSE>)
					.addProperty<int, int>("ThrustPackMinVerticalImpulse", &getPropId<Items::PropId, Items::PropId::THRUST_PACK_MIN_VERTICAL_IMPULSE>)
					.addProperty<int, int>("ThrustPackCooldownTime", &getPropId<Items::PropId, Items::PropId::THRUST_PACK_COOLDOWN_TIME>)
					.addProperty<int, int>("ThrustPackSpeedRangeStart", &getPropId<Items::PropId, Items::PropId::THRUST_PACK_SPEED_RANGE_START>)
					.addProperty<int, int>("ThrustPackSpeedRangeEnd", &getPropId<Items::PropId, Items::PropId::THRUST_PACK_SPEED_RANGE_END>)
					.addProperty<int, int>("ThrustPackSpeedCapReduction", &getPropId<Items::PropId, Items::PropId::THRUST_PACK_SPEED_CAP_REDUCTION>)
					.addProperty<int, int>("ShieldPackEnergyCostPerDamagePoint", &getPropId<Items::PropId, Items::PropId::SHIELD_PACK_ENERGY_COST_PER_DAMAGE_POINT>)
					.addProperty<int, int>("JammerPackRange", &getPropId<Items::PropId, Items::PropId::JAMMER_PACK_RANGE>)
					.addProperty<int, int>("PackBuffAmount", &getPropId<Items::PropId, Items::PropId::PACK_BUFF_AMOUNT>)
					.addProperty<int, int>("StealthPackMaxSpeed", &getPropId<Items::PropId, Items::PropId::STEALTH_PACK_MAX_SPEED>)
					// Deployable
					.addProperty<int, int>("DeployableRange", &getPropId<Items::PropId, Items::PropId::DEPLOYABLE_RANGE>)
					.addProperty<int, int>("DeployableMaxAllowed", &getPropId<Items::PropId, Items::PropId::DEPLOYABLE_MAX_ALLOWED>)
					.addProperty<int, int>("DeployableMinProximity", &getPropId<Items::PropId, Items::PropId::DEPLOYABLE_MIN_PROXIMITY>)
					.addProperty<int, int>("TurretTimeToAcquireTarget", &getPropId<Items::PropId, Items::PropId::TURRET_TIME_TO_ACQUIRE_TARGET>)
					.addProperty<int, int>("TurretCanTargetVehicles", &getPropId<Items::PropId, Items::PropId::TURRET_CAN_TARGET_VEHICLES>)
					// Mines
					.addProperty<int, int>("MineDeployTime", &getPropId<Items::PropId, Items::PropId::MINE_DEPLOY_TIME>)
					.addProperty<int, int>("MineMaxAllowed", &getPropId<Items::PropId, Items::PropId::MINE_MAX_ALLOWED>)
					.addProperty<int, int>("MineCollisionCylinderRadius", &getPropId<Items::PropId, Items::PropId::MINE_COLLISION_CYLINDER_RADIUS>)
					.addProperty<int, int>("MineCollisionCylinderHeight", &getPropId<Items::PropId, Items::PropId::MINE_COLLISION_CYLINDER_HEIGHT>)
					.addProperty<int, int>("ClaymoreDetonationAngle", &getPropId<Items::PropId, Items::PropId::CLAYMORE_DETONATION_ANGLE>)
					.addProperty<int, int>("PrismMineTripDistance", &getPropId<Items::PropId, Items::PropId::PRISM_MINE_TRIP_DISTANCE>)

				.endNamespace()
			.endNamespace()
			.beginNamespace("Classes")
				.addFunction("setProperty", &setClassProp)
				.beginNamespace("Properties")
					// Base stats
					.addProperty<int, int>("HealthPool", &getPropId<Classes::PropId, Classes::PropId::HEALTH_POOL>)
					.addProperty<int, int>("EnergyPool", &getPropId<Classes::PropId, Classes::PropId::ENERGY_POOL>)
					.addProperty<int, int>("EnergyRechargeRate", &getPropId<Classes::PropId, Classes::PropId::ENERGY_RECHARGE_RATE>)
					.addProperty<int, int>("InitialJetEnergyCost", &getPropId<Classes::PropId, Classes::PropId::INITIAL_JET_ENERGY_COST>)
					.addProperty<int, int>("JetEnergyCost", &getPropId<Classes::PropId, Classes::PropId::JET_ENERGY_COST>)
					.addProperty<int, int>("RegenTime", &getPropId<Classes::PropId, Classes::PropId::REGEN_TIME>)
					.addProperty<int, int>("RegenRate", &getPropId<Classes::PropId, Classes::PropId::REGEN_RATE>)
					.addProperty<int, int>("LowHealthThreshold", &getPropId<Classes::PropId, Classes::PropId::LOW_HEALTH_THRESHOLD>)
					// Movement / Skiing
					.addProperty<int, int>("Mass", &getPropId<Classes::PropId, Classes::PropId::MASS>)
					.addProperty<int, int>("GroundSpeed", &getPropId<Classes::PropId, Classes::PropId::GROUND_SPEED>)
					.addProperty<int, int>("MaxSkiingSpeed", &getPropId<Classes::PropId, Classes::PropId::MAX_SKIING_SPEED>)
					.addProperty<int, int>("MaxSkiControl", &getPropId<Classes::PropId, Classes::PropId::MAX_SKI_CONTROL>)
					.addProperty<int, int>("SkiControlPeakSpeed", &getPropId<Classes::PropId, Classes::PropId::SKI_CONTROL_PEAK_SPEED>)
					.addProperty<int, int>("SkiControlVariance", &getPropId<Classes::PropId, Classes::PropId::SKI_CONTROL_VARIANCE>)
					.addProperty<int, int>("SkiSlopeGravity", &getPropId<Classes::PropId, Classes::PropId::SKI_SLOPE_GRAVITY>)
					.addProperty<int, int>("VehicleSpeedInheritance", &getPropId<Classes::PropId, Classes::PropId::VEHICLE_SPEED_INHERITANCE>)
					.addProperty<int, int>("MomentumDampeningEnabled", &getPropId<Classes::PropId, Classes::PropId::MOMENTUM_DAMPENING_ENABLED>)
					.addProperty<int, int>("MomentumDampeningThreshold", &getPropId<Classes::PropId, Classes::PropId::MOMENTUM_DAMPENING_THRESHOLD>)
					.addProperty<int, int>("MomentumDampeningProportion", &getPropId<Classes::PropId, Classes::PropId::MOMENTUM_DAMPENING_PROPORTION>)
					// Jetting / Air Control
					.addProperty<int, int>("MaxJettingSpeed", &getPropId<Classes::PropId, Classes::PropId::MAX_JETTING_SPEED>)
					.addProperty<int, int>("JetAcceleration", &getPropId<Classes::PropId, Classes::PropId::JET_ACCELERATION>)
					.addProperty<int, int>("InitialJetAccelerationMultiplier", &getPropId<Classes::PropId, Classes::PropId::INITIAL_JET_ACCELERATION_MULTIPLIER>)
					.addProperty<int, int>("InitialJetLength", &getPropId<Classes::PropId, Classes::PropId::INITIAL_JET_LENGTH>)
					.addProperty<int, int>("ForwardJetProportion", &getPropId<Classes::PropId, Classes::PropId::FORWARD_JET_PROPORTION>)
					.addProperty<int, int>("JetBoostMaxGroundSpeed", &getPropId<Classes::PropId, Classes::PropId::JET_BOOST_MAX_GROUND_SPEED>)
					.addProperty<int, int>("AirSpeed", &getPropId<Classes::PropId, Classes::PropId::AIR_SPEED>)
					.addProperty<int, int>("AirControl", &getPropId<Classes::PropId, Classes::PropId::AIR_CONTROL>)
					.addProperty<int, int>("AirControlMinMultiplier", &getPropId<Classes::PropId, Classes::PropId::AIR_CONTROL_MIN_MULTIPLIER>)
					.addProperty<int, int>("AirControlReductionRangeMax", &getPropId<Classes::PropId, Classes::PropId::AIR_CONTROL_REDUCTION_RANGE_MAX>)
					.addProperty<int, int>("AirControlReductionRangeMin", &getPropId<Classes::PropId, Classes::PropId::AIR_CONTROL_REDUCTION_RANGE_MIN>)
					// Collision
					.addProperty<int, int>("CollisionCylinderRadius", &getPropId<Classes::PropId, Classes::PropId::COLLISION_CYLINDER_RADIUS>)
					.addProperty<int, int>("CollisionCylinderHeight", &getPropId<Classes::PropId, Classes::PropId::COLLISION_CYLINDER_HEIGHT>)
				.endNamespace()
			.endNamespace()
			.beginNamespace("Vehicles")
				.addFunction("setProperty", &setVehicleProp)
				.beginNamespace("Properties")
					// Base Stats
					.addProperty<int, int>("HealthPool", &getPropId<Vehicles::PropId, Vehicles::PropId::HEALTH_POOL>)
					.addProperty<int, int>("EnergyPool", &getPropId<Vehicles::PropId, Vehicles::PropId::ENERGY_POOL>)
					.addProperty<int, int>("EnergyRechargeRate", &getPropId<Vehicles::PropId, Vehicles::PropId::ENERGY_RECHARGE_RATE>)
					.addProperty<int, int>("IsArmored", &getPropId<Vehicles::PropId, Vehicles::PropId::IS_ARMORED>)
					.addProperty<int, int>("IsArmoured", &getPropId<Vehicles::PropId, Vehicles::PropId::IS_ARMORED>)
					.addProperty<int, int>("IsHomingTarget", &getPropId<Vehicles::PropId, Vehicles::PropId::IS_HOMING_TARGET>)
					.addProperty<int, int>("CanCarryFlagAsPilot", &getPropId<Vehicles::PropId, Vehicles::PropId::CAN_CARRY_FLAG_AS_PILOT>)
					.addProperty<int, int>("CanCarryFlagAsPassenger", &getPropId<Vehicles::PropId, Vehicles::PropId::CAN_CARRY_FLAG_AS_PASSENGER>)
					.addProperty<int, int>("TimeBeforeSelfDestruct", &getPropId<Vehicles::PropId, Vehicles::PropId::TIME_BEFORE_SELFDESTRUCT>)
					// Movement
					.addProperty<int, int>("MaxSpeed", &getPropId<Vehicles::PropId, Vehicles::PropId::MAX_SPEED>)
					.addProperty<int, int>("MaxDivingSpeedMultiplier", &getPropId<Vehicles::PropId, Vehicles::PropId::MAX_DIVING_SPEED_MULTIPLIER>)
					.addProperty<int, int>("BoostMultiplier", &getPropId<Vehicles::PropId, Vehicles::PropId::BOOST_MULTIPLIER>)
					.addProperty<int, int>("BoostEnergyCost", &getPropId<Vehicles::PropId, Vehicles::PropId::BOOST_ENERGY_COST>)
					.addProperty<int, int>("BoostMinUsableProportion", &getPropId<Vehicles::PropId, Vehicles::PropId::BOOST_MIN_USABLE_PROPORTION>)
					.addProperty<int, int>("MaxPlayerExitSpeed", &getPropId<Vehicles::PropId, Vehicles::PropId::MAX_PLAYER_EXIT_SPEED>)
					.addProperty<int, int>("GravityScale", &getPropId<Vehicles::PropId, Vehicles::PropId::GRAVITY_SCALE>)
					// Self-Damage
					.addProperty<int, int>("MaxCrashDamage", &getPropId<Vehicles::PropId, Vehicles::PropId::MAX_CRASH_DAMAGE>)
					.addProperty<int, int>("MinCrashDamage", &getPropId<Vehicles::PropId, Vehicles::PropId::MIN_CRASH_DAMAGE>)
					.addProperty<int, int>("MaxCrashDamageSpeed", &getPropId<Vehicles::PropId, Vehicles::PropId::MAX_CRASH_DAMAGE_SPEED>)
					.addProperty<int, int>("MinCrashDamageSpeed", &getPropId<Vehicles::PropId, Vehicles::PropId::MIN_CRASH_DAMAGE_SPEED>)
					.addProperty<int, int>("MaxVehicleCrashDamage", &getPropId<Vehicles::PropId, Vehicles::PropId::MAX_VEHICLE_CRASH_DAMAGE>)
					.addProperty<int, int>("MinVehicleCrashDamage", &getPropId<Vehicles::PropId, Vehicles::PropId::MIN_VEHICLE_CRASH_DAMAGE>)
					.addProperty<int, int>("MaxVehicleCrashDamageSpeed", &getPropId<Vehicles::PropId, Vehicles::PropId::MAX_VEHICLE_CRASH_DAMAGE_SPEED>)
					.addProperty<int, int>("MinVehicleCrashDamageSpeed", &getPropId<Vehicles::PropId, Vehicles::PropId::MIN_VEHICLE_CRASH_DAMAGE_SPEED>)
					// Ramming
					.addProperty<int, int>("RamMinSpeed", &getPropId<Vehicles::PropId, Vehicles::PropId::RAM_MIN_SPEED>)
					.addProperty<int, int>("RamMaxDamage", &getPropId<Vehicles::PropId, Vehicles::PropId::RAM_MAX_DAMAGE>)
					.addProperty<int, int>("RamMinDamage", &getPropId<Vehicles::PropId, Vehicles::PropId::RAM_MIN_DAMAGE>)
					.addProperty<int, int>("RamMaxDamageSpeed", &getPropId<Vehicles::PropId, Vehicles::PropId::RAM_MAX_DAMAGE_SPEED>)
					.addProperty<int, int>("RamPlayerPushSpeed", &getPropId<Vehicles::PropId, Vehicles::PropId::RAM_PLAYER_PUSH_SPEED>)
					.addProperty<int, int>("RamFlagPushSpeed", &getPropId<Vehicles::PropId, Vehicles::PropId::RAM_FLAG_PUSH_SPEED>)
				.endNamespace()
			.endNamespace()
			.beginNamespace("VehicleWeapons")
				.addFunction("setProperty", &setVehicleWeaponProp)
				.beginNamespace("Properties")
					// Ammo
					.addProperty<int, int>("ClipAmmo", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::CLIP_AMMO>)
					.addProperty<int, int>("SpareAmmo", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::SPARE_AMMO>)
					.addProperty<int, int>("AmmoPerShot", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::AMMO_PER_SHOT>)
					// Reload / Firing
					.addProperty<int, int>("ReloadTime", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::RELOAD_TIME>)
					.addProperty<int, int>("FireInterval", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::FIRE_INTERVAL>)
					.addProperty<int, int>("ReloadSingle", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::RELOAD_SINGLE>)
					.addProperty<int, int>("ReloadApplicationProportion", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::RELOAD_APPLICATION_PROPORTION>)
					.addProperty<int, int>("BurstShotCount", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::BURST_SHOT_COUNT>)
					// Damage / Impact
					.addProperty<int, int>("Damage", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DAMAGE>)
					.addProperty<int, int>("ExplosiveRadius", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::EXPLOSIVE_RADIUS>)
					.addProperty<int, int>("DirectHitMultiplier", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DIRECT_HIT_MULTIPLIER>)
					.addProperty<int, int>("MomentumTransfer", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::MOMENTUM_TRANSFER>)
					.addProperty<int, int>("EnergyDrain", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::ENERGY_DRAIN>)
					.addProperty<int, int>("MaxDamageRangeProportion", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::MAX_DAMAGE_RANGE_PROPORTION>)
					.addProperty<int, int>("MinDamageRangeProportion", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::MIN_DAMAGE_RANGE_PROPORTION>)
					.addProperty<int, int>("MinDamageProportion", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::MIN_DAMAGE_PROPORTION>)
					.addProperty<int, int>("BulletDamageRange", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::BULLET_DAMAGE_RANGE>)
					.addProperty<int, int>("DamageAgainstArmorMultiplier", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DAMAGE_AGAINST_ARMOR_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstGeneratorMultiplier", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DAMAGE_AGAINST_GENERATOR_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstBaseTurretMultiplier", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DAMAGE_AGAINST_BASE_TURRET_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstObjectiveMultiplier", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DAMAGE_AGAINST_BASE_SENSOR_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstGravCycleMultiplier", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DAMAGE_AGAINST_GRAVCYCLE_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstBeowulfMultiplier", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DAMAGE_AGAINST_BEOWULF_MULTIPLIER>)
					.addProperty<int, int>("DamageAgainstShrikeMultiplier", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DAMAGE_AGAINST_SHRIKE_MULTIPLIER>)
					.addProperty<int, int>("DoesGibOnKill", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DOES_GIB_ON_KILL>)
					.addProperty<int, int>("GibImpulseRadius", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::GIB_IMPULSE_RADIUS>)
					.addProperty<int, int>("GibStrength", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::GIB_STRENGTH>)
					.addProperty<int, int>("DoesImpulseFlag", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::DOES_IMPULSE_FLAG>)
					// Projectile
					.addProperty<int, int>("ProjectileSpeed", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::PROJECTILE_SPEED>)
					.addProperty<int, int>("ProjectileMaxSpeed", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::PROJECTILE_MAX_SPEED>)
					.addProperty<int, int>("CollisionSize", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::COLLISION_SIZE>)
					.addProperty<int, int>("ProjectileInheritence", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::PROJECTILE_INHERITANCE>)
					.addProperty<int, int>("ProjectileLifespan", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::PROJECTILE_LIFESPAN>)
					.addProperty<int, int>("ProjectileGravity", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::PROJECTILE_GRAVITY>)
					.addProperty<int, int>("ProjectileTerminalVelocity", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::PROJECTILE_TERMINAL_VELOCITY>)
					// Accuracy
					.addProperty<int, int>("Accuracy", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::ACCURACY>)
					.addProperty<int, int>("AccuracyLossOnShot", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::ACCURACY_LOSS_ON_SHOT>)
					.addProperty<int, int>("AccuracyLossMax", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::ACCURACY_LOSS_MAX>)
					.addProperty<int, int>("AccuracyCorrectionRate", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::ACCURACY_CORRECTION_RATE>)
				.endNamespace()
			.endNamespace()
			;
	}
}