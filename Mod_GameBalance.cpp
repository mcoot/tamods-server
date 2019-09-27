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
        sendGameBalanceDetailsMessage(pconn,
            g_config.getReplicatedSettings(),
            g_config.serverSettings.weaponProperties,
            g_config.serverSettings.deviceValueProperties,
            g_config.serverSettings.classProperties,
            g_config.serverSettings.vehicleProperties,
            g_config.serverSettings.vehicleWeaponProperties
        );
    }
}

template <typename BaseClass>
static std::vector<UObject*> getDefaultObjects(std::map<int, std::string>& relevantClassNames, std::string prefix, std::vector<std::string> variants, int elemId) {
    auto& name_it = relevantClassNames.find(elemId);
    if (name_it == relevantClassNames.end()) return std::vector<UObject*>();

    std::string name = name_it->second;

    std::vector<std::string> toFind;
    if (variants.empty()) {
        toFind.push_back(prefix + "_" + name + " TribesGame.Default__" + prefix + "_" + name);
    } else {
        for (auto& v : variants) {
            toFind.push_back(prefix + "_" + name + (v.empty() ? "" : "_") + v + " TribesGame.Default__" + prefix + "_" + name + (v.empty() ? "" : "_") + v);
        }
    }

    std::vector<UObject*> found;

    for (auto& n : toFind) {
        UObject* obj = UObject::FindObject<BaseClass>(n.c_str());
        //Logger::debug("Fetch for object %s; result: %d", n.c_str(), obj);
        if (obj) {
            found.push_back(obj);
        }
        else {
            Logger::debug("Failed to find %s", n.c_str());
        }
    }

    return found;
}

template <typename IdType>
static std::vector<UObject*> getDefaultObjectsForProps(int elemId) {
    bool isItemsCase = std::is_same<IdType, Items::PropId>::value;
    bool isMeleeCase = isItemsCase && elemId == CONST_WEAPON_ID_MELEE; // Melee is special cased because it has BE and DS variants
    bool isClassCase = std::is_same<IdType, Classes::PropId>::value;
    bool isVehicleCase = std::is_same<IdType, Vehicles::PropId>::value;
    bool isVehicleWeaponCase = std::is_same<IdType, VehicleWeapons::PropId>::value;
    
    std::string prefix;
    std::vector<std::string> variants;
    std::map<int, std::string> relevantClassNames;
    if (isItemsCase) {
        prefix = "TrDevice";
        relevantClassNames = Data::item_id_to_name;
        if (isMeleeCase) {
            variants.push_back("");
            variants.push_back("BE");
            variants.push_back("DS");
            variants.push_back("MC");
        }
        if (Data::perk_id_to_name.find(elemId) != Data::perk_id_to_name.end()) {
            prefix = "TrPerk";
        }
        return getDefaultObjects<ATrDevice>(relevantClassNames, prefix, variants, elemId);
    }
    else if (isClassCase) {
        prefix = "TrFamilyInfo";
        relevantClassNames = Data::armor_class_id_to_name;
        variants.push_back("");
        variants.push_back("BE");
        variants.push_back("DS");
        return getDefaultObjects<UTrFamilyInfo>(relevantClassNames, prefix, variants, elemId);
    }
    else if (isVehicleCase) {
        prefix = "TrVehicle";
        relevantClassNames = Data::vehicle_id_to_name;
        return getDefaultObjects<ATrVehicle>(relevantClassNames, prefix, variants, elemId);
    }
    else if (isVehicleWeaponCase) {
        prefix = "TrVehicleWeapon";
        relevantClassNames = Data::vehicle_weapon_id_to_name;
        return getDefaultObjects<ATrVehicleWeapon>(relevantClassNames, prefix, variants, elemId);
    }

    return std::vector<UObject*>();
}

template <typename IdType>
static void applyPropConfig(std::map<IdType, Property>& propDefs, std::map<int, std::map<IdType, PropValue> >& props) {
    for (auto& elem : props) {
        // Get the default objects to apply to
        std::vector<UObject*> objectsToApplyOn = getDefaultObjectsForProps<IdType>(elem.first);
        if (objectsToApplyOn.empty()) {
            Logger::error("Unable to set properties; failed to get a default object for id %d", elem.first);
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

static void applyValueModConfig(Items::DeviceValuesConfig& config) {
    for (auto& elem : config) {
        // Get the object/s needed to modify
        // Item is either a regular item or an armour mod; in the former case it will have a class id rather than an item ID
        std::vector<UObject*> objects;
        if (Data::armor_class_id_to_armor_mod_name.find(elem.first) != Data::armor_class_id_to_armor_mod_name.end()) {
            objects = getDefaultObjects<ATrArmorMod>(Data::armor_class_id_to_armor_mod_name, "TrArmorMod", std::vector<std::string>(), elem.first);
        }
        else {
            objects = getDefaultObjectsForProps<Items::PropId>(elem.first);
        }

        for (auto& obj : objects) {
            // All objects should be Devices
            ATrDevice* dev = (ATrDevice*)obj;

            // Reset existing modifications
            // Here I'm deliberately leaking memory by losing the reference to the existing BaseMod.Modifications data
            // (since TArray doesn't have a destructor that frees it)
            // Actually freeing the original data intermittently causes crashes (maybe due to UnrealScript GC interaction?)
            // The amount of memory leaked should hopefully not be too large;
            // On server, it will leak: 8 bytes * no. of mods on device * no. times setValueMods is called
            // On client, it will leak that many for every server join
            dev->BaseMod.Modifications = TArray<FDeviceModification>();
            // Apply modifications
            for (DeviceValueMod& mod : elem.second) {
                FDeviceModification devMod;
                devMod.ModType = mod.modType;
                devMod.Value = mod.value;
                dev->BaseMod.Modifications.Add(devMod);
            }
        }
    }
}

void ServerSettings::ApplyGameBalanceProperties() {
    applyPropConfig(Items::properties, weaponProperties);
    applyValueModConfig(deviceValueProperties);
    applyPropConfig(Classes::properties, classProperties);
    applyPropConfig(Vehicles::properties,vehicleProperties);
    applyPropConfig(VehicleWeapons::properties, vehicleWeaponProperties);
}

template <typename IdType>
static LuaRef getProp(std::map<IdType, Property>& propDefs, std::map<IdType, PropValue>& props, int elemId, int intPropId) {
    std::vector<UObject*> defObjs = getDefaultObjectsForProps<IdType>(elemId);
    if (defObjs.empty()) {
        Logger::error("Unable to get property %d; could not get default objects for item %d", intPropId, elemId);
        return LuaRef(g_config.lua.getState());
    }
    
    // Use the first object returned; they ought to all be the same
    UObject* objToReadFrom = defObjs[0];

    auto& it = propDefs.find((IdType)intPropId);
    if (it == propDefs.end()) {
        // Non-existent property, fail
        Logger::error("Unable to get property; invalid property id %d", intPropId);
        return LuaRef(g_config.lua.getState());
    }
    GameBalance::PropValue val;

    // Find the value of this property in the map
    // And set it based on that value
    auto& pvalIt = props.find((IdType)intPropId);
    if (pvalIt != props.end()) {
        val = pvalIt->second;
    }

    // For most properties, the value is a proxy to an underlying game value
    // Grab the 'true' value from that
    if (!it->second.get(objToReadFrom, val)) {
        // Failed get
        Logger::error("Failed to get property with id %d", intPropId);
        return LuaRef(g_config.lua.getState());
    }

    return val.getAsLuaRef(g_config.lua.getState());
}

static LuaRef getValueMod(int elemId) {
    std::vector<UObject*> objects;
    std::string kindForErrorString;
    if (Data::armor_class_id_to_armor_mod_name.find(elemId) != Data::armor_class_id_to_armor_mod_name.end()) {
        objects = getDefaultObjects<ATrArmorMod>(Data::armor_class_id_to_armor_mod_name, "TrArmorMod", std::vector<std::string>(), elemId);
        kindForErrorString = "armor class";
    }
    else {
        objects = getDefaultObjectsForProps<Items::PropId>(elemId);
        kindForErrorString = "item";
    }
    if (objects.empty()) {
        Logger::error("Failed to get object for %s with id %d", kindForErrorString.c_str(), elemId);
        return LuaRef(g_config.lua.getState());
    }

    ATrDevice* dev = (ATrDevice*)objects[0];
    
    LuaRef out = newTable(g_config.lua.getState());
    for (int i = 0; i < dev->BaseMod.Modifications.Count; ++i) {
        LuaRef curMod = newTable(g_config.lua.getState());

        curMod[1] = dev->BaseMod.Modifications.GetStd(i).ModType;
        curMod[2] = dev->BaseMod.Modifications.GetStd(i).Value;

        out.append(curMod);
    }

    return out;
}

static LuaRef getWeaponProp(std::string className, std::string itemName, int intPropId) {
    int itemId = Data::getItemId(className, itemName);
    if (itemId == 0) {
        Logger::error("Unable to get property config; invalid item %s on class %s", itemName.c_str(), className.c_str());
        return LuaRef(g_config.lua.getState());
    }

    Items::PropMapping propMap;
    auto& it = g_config.serverSettings.weaponProperties.find(itemId);
    if (it != g_config.serverSettings.weaponProperties.end()) {
        propMap = it->second;
    }

    return getProp(Items::properties, propMap, itemId, intPropId);
}

static LuaRef getClassProp(std::string className, int intPropId) {
    int classId = Utils::searchMapId(Data::armor_class_to_id, className, "", false);
    if (classId == 0) {
        Logger::error("Unable to get property config; invalid class %s", className.c_str());
        return LuaRef(g_config.lua.getState());
    }

    Classes::PropMapping propMap;
    auto& it = g_config.serverSettings.classProperties.find(classId);
    if (it != g_config.serverSettings.classProperties.end()) {
        propMap = it->second;
    }

    return getProp(Classes::properties, propMap, classId, intPropId);
}

static LuaRef getVehicleProp(std::string vehicleName, int intPropId) {
    int vehicleId = Utils::searchMapId(Data::vehicles, vehicleName, "", false);
    if (vehicleId == 0) {
        Logger::error("Unable to get property config; invalid vehicle %s", vehicleName.c_str());
        return LuaRef(g_config.lua.getState());
    }

    Vehicles::PropMapping propMap;
    auto& it = g_config.serverSettings.vehicleProperties.find(vehicleId);
    if (it != g_config.serverSettings.vehicleProperties.end()) {
        propMap = it->second;
    }

    return getProp(Vehicles::properties, propMap, vehicleId, intPropId);
}

static LuaRef getVehicleWeaponProp(std::string vehicleWeaponName, int intPropId) {
    int vehicleWeaponId = Utils::searchMapId(Data::vehicle_weapons, vehicleWeaponName, "", false);
    if (vehicleWeaponId == 0) {
        Logger::error("Unable to get property config; invalid vehicle %s", vehicleWeaponName.c_str());
        return LuaRef(g_config.lua.getState());
    }

    VehicleWeapons::PropMapping propMap;
    auto& it = g_config.serverSettings.vehicleWeaponProperties.find(vehicleWeaponId);
    if (it != g_config.serverSettings.vehicleWeaponProperties.end()) {
        propMap = it->second;
    }

    return getProp(VehicleWeapons::properties, propMap, vehicleWeaponId, intPropId);
}

static LuaRef getDeviceValueMod(std::string className, std::string itemName) {
    int itemId = Data::getItemId(className, itemName);
    if (itemId == 0) {
        Logger::error("Unable to get value mod config; invalid item %s on class %s", className.c_str(), itemName.c_str());
        return LuaRef(g_config.lua.getState());;
    }

    return getValueMod(itemId);
}

static LuaRef getDeviceValueMod_ArmorMod(std::string armorClassName) {
    int classId = Utils::searchMapId(Data::armor_class_to_id, armorClassName, "", false);
    if (classId == 0) {
        Logger::error("Unable to get value mod config; invalid armor class %s", armorClassName.c_str());
        return LuaRef(g_config.lua.getState());;
    }

    return getValueMod(classId);
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

static void setValueMod(Items::DeviceValuesConfig& config, int elemId, LuaRef modDefs) {
    if (!modDefs.isTable()) {
        Logger::error("Invalid type for value mods; should be a table of mod definitions");
        return;
    }

    auto& cit = config.find(elemId);
    if (cit == config.end()) {
        config[elemId] = std::vector<DeviceValueMod>();
    }

    for (int i = 1; i <= modDefs.length(); ++i) {
        LuaRef curModDef = LuaRef(modDefs[i]);
        if (curModDef.isNil()) {
            Logger::error("Failed to set value mods on device %d, index %d; table of value mods should be an array (i.e. be 0-indexed by continguous integer keys)", elemId, i);
            return;
        }
        if (!curModDef.isTable()) {
            Logger::error("Failed to set value mods on device %d, index %d; value mod was not a table", elemId, i);
            return;
        }

        LuaRef curModType = LuaRef(curModDef[1]);
        LuaRef curModValue = LuaRef(curModDef[2]);

        if (curModType.isNil() || curModValue.isNil()) {
            Logger::error("Failed to set value mods on device %d, index %d; value mod should be a two-element array containing the mod type and the value", elemId, i);
            return;
        }

        float value;
        if (curModValue.isNumber()) {
            value = curModValue;
        }
        else if (curModValue.type() == 1) {
            value = static_cast<bool>(curModValue) ? 1.0f : 0.0f;
        }
        else {
            Logger::error("Failed to set value mods on device %d, index %d; value mod should be a number or boolean", elemId, i);
            return;
        }

        DeviceValueMod mod;
        mod.modType = curModType;
        mod.value = value;
        config[elemId].push_back(mod);
    }    
}

static void setWeaponProp(std::string className, std::string itemName, int intPropId, LuaRef val) {
    int itemId = Data::getItemId(className, itemName);
    if (itemId == 0) {
        Logger::error("Unable to set property config; invalid item %s on class %s", className.c_str(), itemName.c_str());
        return;
    }

    setProp(Items::properties, g_config.serverSettings.weaponProperties, itemId, intPropId, val);
}

static void setClassProp(std::string className, int intPropId, LuaRef val) {
    int classId = Utils::searchMapId(Data::armor_class_to_id, className, "", false);
    if (classId == 0) {
        Logger::error("Unable to set property config; invalid class %s", className.c_str());
        return;
    }

    setProp(Classes::properties, g_config.serverSettings.classProperties, classId, intPropId, val);
}

static void setVehicleProp(std::string vehicleName, int intPropId, LuaRef val) {
    int vehicleId = Utils::searchMapId(Data::vehicles, vehicleName, "", false, -1);
    if (vehicleId == -1) {
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

static void setDeviceValueMod(std::string className, std::string itemName, LuaRef modDefs) {
    int itemId = Data::getItemId(className, itemName);
    if (itemId == 0) {
        Logger::error("Unable to set value mod config; invalid item %s on class %s", className.c_str(), itemName.c_str());
        return;
    }

    setValueMod(g_config.serverSettings.deviceValueProperties, itemId, modDefs);
}

static void setDeviceValueMod_ArmorMod(std::string armorClassName, LuaRef modDefs) {
    int classId = Utils::searchMapId(Data::armor_class_to_id, armorClassName, "", false);
    if (classId == 0) {
        Logger::error("Unable to set value mod config; invalid armor class %s", armorClassName.c_str());
        return;
    }

    setValueMod(g_config.serverSettings.deviceValueProperties, classId, modDefs);
}

template <typename IdType, IdType x>
static int getPropId() {
    return (int)x;
}

template <int x>
static int getIdentity() {
    return x;
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
                .addFunction("setValueMods", &setDeviceValueMod)
                .addFunction("getValueMods", &getDeviceValueMod)
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
                    .addProperty<int, int>("BurstShotCount", &getPropId<Items::PropId, Items::PropId::BURST_SHOT_COUNT>)
                    .addProperty<int, int>("BurstShotRefireTime", &getPropId<Items::PropId, Items::PropId::BURST_SHOT_REFIRE_TIME>)
                    .addProperty<int, int>("SpinupTime", &getPropId<Items::PropId, Items::PropId::SPINUP_TIME>)
                    .addProperty<int, int>("ShotgunShotCount", &getPropId<Items::PropId, Items::PropId::SHOTGUN_SHOT_COUNT>)
                    .addProperty<int, int>("ShotEnergyCost", &getPropId<Items::PropId, Items::PropId::SHOT_ENERGY_COST>)
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
                    .addProperty<int, int>("PhaseDamagePerEnergy", &getPropId<Items::PropId, Items::PropId::PHASE_DAMAGE_PER_ENERGY>)
                    .addProperty<int, int>("PhaseMaxConsumedEnergy", &getPropId<Items::PropId, Items::PropId::PHASE_MAX_CONSUMED_ENERGY>)
                    .addProperty<int, int>("BXTChargeMaxDamage", &getPropId<Items::PropId, Items::PropId::BXT_CHARGE_MAX_DAMAGE>)
                    .addProperty<int, int>("BXTChargeTime", &getPropId<Items::PropId, Items::PropId::BXT_CHARGE_TIME>)
                    .addProperty<int, int>("BXTChargeMultCoefficient", &getPropId<Items::PropId, Items::PropId::BXT_CHARGE_MULT_COEFFICIENT>)
                    .addProperty<int, int>("BXTChargeDivCoefficient", &getPropId<Items::PropId, Items::PropId::BXT_CHARGE_DIV_COEFFICIENT>)
                    .addProperty<int, int>("FractalDuration", &getPropId<Items::PropId, Items::PropId::FRACTAL_DURATION>)
                    .addProperty<int, int>("FractalShardInterval", &getPropId<Items::PropId, Items::PropId::FRACTAL_SHARD_INTERVAL>)
                    .addProperty<int, int>("FractalAscentTime", &getPropId<Items::PropId, Items::PropId::FRACTAL_ASCENT_TIME>)
                    .addProperty<int, int>("FractalAscentHeight", &getPropId<Items::PropId, Items::PropId::FRACTAL_ASCENT_HEIGHT>)
                    .addProperty<int, int>("FractalShardDistance", &getPropId<Items::PropId, Items::PropId::FRACTAL_SHARD_DISTANCE>)
                    .addProperty<int, int>("FractalShardHeight", &getPropId<Items::PropId, Items::PropId::FRACTAL_SHARD_HEIGHT>)
                    .addProperty<int, int>("FractalShardDamage", &getPropId<Items::PropId, Items::PropId::FRACTAL_SHARD_DAMAGE>)
                    .addProperty<int, int>("FractalShardDamageRadius", &getPropId<Items::PropId, Items::PropId::FRACTAL_SHARD_DAMAGE_RADIUS>)
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
                    .addProperty<int, int>("ShotgunInnerAccuracy", &getPropId<Items::PropId, Items::PropId::SHOTGUN_INNER_ACCURACY>)
                    .addProperty<int, int>("ShotgunUseGOTYSpread", &getPropId<Items::PropId, Items::PropId::SHOTGUN_USE_GOTY_SPREAD>)
                    // Grenade
                    .addProperty<int, int>("ThrowDelay", &getPropId<Items::PropId, Items::PropId::THROW_DELAY>)
                    .addProperty<int, int>("ThrowPullPinTime", &getPropId<Items::PropId, Items::PropId::THROW_PULL_PIN_TIME>)
                    .addProperty<int, int>("StuckDamageMultiplier", &getPropId<Items::PropId, Items::PropId::STUCK_DAMAGE_MULTIPLIER>)
                    .addProperty<int, int>("StuckMomentumMultiplier", &getPropId<Items::PropId, Items::PropId::STUCK_MOMENTUM_MULTIPLIER>)
                    .addProperty<int, int>("FuseTimer", &getPropId<Items::PropId, Items::PropId::FUSE_TIMER>)
                    .addProperty<int, int>("ExplodeOnContact", &getPropId<Items::PropId, Items::PropId::EXPLODE_ON_CONTACT>)
                    .addProperty<int, int>("ExplodeOnFuse", &getPropId<Items::PropId, Items::PropId::EXPLODE_ON_FUSE>)
                    .addProperty<int, int>("MustBounceBeforeExplode", &getPropId<Items::PropId, Items::PropId::MUST_BOUNCE_BEFORE_EXPLODE>)
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
                    .addProperty<int, int>("ForcefieldMinDamage", &getPropId<Items::PropId, Items::PropId::FORCEFIELD_MIN_DAMAGE>)
                    .addProperty<int, int>("ForcefieldMaxDamage", &getPropId<Items::PropId, Items::PropId::FORCEFIELD_MAX_DAMAGE>)
                    .addProperty<int, int>("ForcefieldMinDamageSpeed", &getPropId<Items::PropId, Items::PropId::FORCEFIELD_MIN_DAMAGE_SPEED>)
                    .addProperty<int, int>("ForcefieldMaxDamageSpeed", &getPropId<Items::PropId, Items::PropId::FORCEFIELD_MAX_DAMAGE_SPEED>)
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
                .addFunction("getProperty", &getClassProp)
                .addFunction("setValueMods", &setDeviceValueMod_ArmorMod)
                .addFunction("getValueMods", &getDeviceValueMod_ArmorMod)
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
                    .addProperty<int, int>("DefaultAirControl", &getPropId<Classes::PropId, Classes::PropId::DEFAULT_AIR_CONTROL>)
                    .addProperty<int, int>("AirControlMaxMultiplier", &getPropId<Classes::PropId, Classes::PropId::AIR_CONTROL_MAX_MULTIPLIER>)
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
                .addFunction("getProperty", &getVehicleProp)
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
                .addFunction("getProperty", &getVehicleWeaponProp)
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
                    .addProperty<int, int>("ImpactMomentum", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::IMPACT_MOMENTUM>)
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
                    .addProperty<int, int>("ProjectileInheritance", &getPropId<VehicleWeapons::PropId, VehicleWeapons::PropId::PROJECTILE_INHERITANCE>)
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
            .beginNamespace("ValueMods")
                .addProperty<int, int>("None", &getIdentity<CONST_MOD_TYPE_NONE>)
                .addProperty<int, int>("HealthBuff", &getIdentity<CONST_MOD_TYPE_HEALTH>)
                .addProperty<int, int>("MassBuff", &getIdentity<CONST_MOD_TYPE_MASSPCT>)
                .addProperty<int, int>("RadarLink", &getIdentity<CONST_MOD_TYPE_RADARLINK>)
                .addProperty<int, int>("Reach", &getIdentity<CONST_MOD_TYPE_FLAGREACH>)
                .addProperty<int, int>("BuildTimeBuff", &getIdentity<CONST_MOD_TYPE_BUILDUPPCT>)
                .addProperty<int, int>("Clothesline", &getIdentity<CONST_MOD_TYPE_CLOTHESLINE>)
                .addProperty<int, int>("SuperHeavy", &getIdentity<CONST_MOD_TYPE_CLOTHESLINE>)
                .addProperty<int, int>("StickyHands", &getIdentity<CONST_MOD_TYPE_STICKYHANDS>)
                .addProperty<int, int>("MaxSkiSpeed", &getIdentity<CONST_MOD_TYPE_MAXSKISPEED>)
                .addProperty<int, int>("EnergyBuff", &getIdentity<CONST_MOD_TYPE_EXTRAENERGY>)
                .addProperty<int, int>("EjectionSeat", &getIdentity<CONST_MOD_TYPE_EJECTIONSEAT>)
                .addProperty<int, int>("Pilot", &getIdentity<CONST_MOD_TYPE_EJECTIONSEAT>)
                .addProperty<int, int>("SelfDamageReduction", &getIdentity<CONST_MOD_TYPE_SELFDAMAGEPCT>)
                .addProperty<int, int>("Egocentric", &getIdentity<CONST_MOD_TYPE_SELFDAMAGEPCT>)
                .addProperty<int, int>("RepairRateBuff", &getIdentity<CONST_MOD_TYPE_REPAIRRATEPCT>)
                .addProperty<int, int>("ExtraMines", &getIdentity<CONST_MOD_TYPE_EXTRAMINESOUT>)
                .addProperty<int, int>("VehicleHealthBuff", &getIdentity<CONST_MOD_TYPE_VEHICLEEXTRAHEALTH>)
                .addProperty<int, int>("WalkSpeedBuff", &getIdentity<CONST_MOD_TYPE_GROUNDSPEEDPCT>)
                .addProperty<int, int>("EnergyDrainBuff", &getIdentity<CONST_MOD_TYPE_ENERGYDRAINPCT>)
                .addProperty<int, int>("UpgradeCostReduction", &getIdentity<CONST_MOD_TYPE_UPGRADECOSTPCT>)
                .addProperty<int, int>("TrapDetection", &getIdentity<CONST_MOD_TYPE_CANDETECTTRAPS>)
                .addProperty<int, int>("FallDamageReduction", &getIdentity<CONST_MOD_TYPE_SPLATDAMAGEPCT>)
                .addProperty<int, int>("SafeFall", &getIdentity<CONST_MOD_TYPE_SPLATDAMAGEPCT>)
                .addProperty<int, int>("QuickDraw", &getIdentity<CONST_MOD_TYPE_WEAPONSWITCHPCT>)
                .addProperty<int, int>("PotentialEnergy", &getIdentity<CONST_MOD_TYPE_POTENTIALENERGY>)
                .addProperty<int, int>("MaxJetSpeed", &getIdentity<CONST_MOD_TYPE_MAXJETTINGSPEED>)
                .addProperty<int, int>("CreditsFromKillsBuff", &getIdentity<CONST_MOD_TYPE_CREDITSFROMKILLS>)
                .addProperty<int, int>("TerminalSkiSpeed", &getIdentity<CONST_MOD_TYPE_TERMINALSKISPEED>)
                .addProperty<int, int>("MaxSkiControl", &getIdentity<CONST_MOD_TYPE_MAXSKICONTROLPCT>)
                .addProperty<int, int>("Determination", &getIdentity<CONST_MOD_TYPE_HASDETERMINATION>)
                .addProperty<int, int>("DeployableHealthBuff", &getIdentity<CONST_MOD_TYPE_DEPLOYABLEHEALTH>)
                .addProperty<int, int>("ExtraBeltAmmo", &getIdentity<CONST_MOD_TYPE_EXTRAOFFHANDAMMO>)
                .addProperty<int, int>("SafetyThird", &getIdentity<CONST_MOD_TYPE_EXTRAOFFHANDAMMO>)
                .addProperty<int, int>("ExtraPrimaryAmmo", &getIdentity<CONST_MOD_TYPE_EXTRAPRIMARYAMMO>)
                .addProperty<int, int>("PrimaryReloadTimeBuff", &getIdentity<CONST_MOD_TYPE_PRIMARYRELOADPCT>)
                .addProperty<int, int>("SensorDetectionReduction", &getIdentity<CONST_MOD_TYPE_SENSORDISTANCEPCT>)
                .addProperty<int, int>("Stealthy", &getIdentity<CONST_MOD_TYPE_SENSORDISTANCEPCT>)
                .addProperty<int, int>("ThrustPackCostReduction", &getIdentity<CONST_MOD_TYPE_PACKENERGYCOSTPCT>)
                .addProperty<int, int>("AmmoPickupBuff", &getIdentity<CONST_MOD_TYPE_AMMOFROMPICKUPPCT>)
                .addProperty<int, int>("EnergyRegenTimeReduction", &getIdentity<CONST_MOD_TYPE_TIMETOREGENENERGY>)
                .addProperty<int, int>("EnergyRegenTimeBuff", &getIdentity<CONST_MOD_TYPE_TIMETOREGENENERGY>)
                .addProperty<int, int>("SecondaryReloadTimeBuff", &getIdentity<CONST_MOD_TYPE_SECONDARYRELOADPCT>)
                .addProperty<int, int>("ExtraSecondaryAmmo", &getIdentity<CONST_MOD_TYPE_EXTRASECONDARYAMMO>)
                .addProperty<int, int>("EnergyRegenRateBuff", &getIdentity<CONST_MOD_TYPE_ENERGYREGENRATEPCT>)
                .addProperty<int, int>("HealthRegenRateBuff", &getIdentity<CONST_MOD_TYPE_HEALTHREGENRATEPCT>)
                .addProperty<int, int>("DeployableRangeBuff", &getIdentity<CONST_MOD_TYPE_DEPLOYABLERANGEPCT>)
                .addProperty<int, int>("JammerPackRadiusBuff", &getIdentity<CONST_MOD_TYPE_JAMMERPACKRADIUSPCT>)
                .addProperty<int, int>("ThrustPackPowerBuff", &getIdentity<CONST_MOD_TYPE_BLINKPACKPOTENCYPCT>)
                .addProperty<int, int>("PeakSkiControlSpeed", &getIdentity<CONST_MOD_TYPE_PEAKSKICONTROLSPEED>)
                .addProperty<int, int>("CanCallInSupplyDrop", &getIdentity<CONST_MOD_TYPE_CANCALLINSUPPLYDROP>)
                .addProperty<int, int>("ExtraDeployables", &getIdentity<CONST_MOD_TYPE_EXTRADEPLOYABLESOUT>)
                .addProperty<int, int>("PickupHealthBuff", &getIdentity<CONST_MOD_TYPE_HEALTHFROMPICKUPPCT>)
                .addProperty<int, int>("SurvivalistHealth", &getIdentity<CONST_MOD_TYPE_HEALTHFROMPICKUPPCT>)
                .addProperty<int, int>("PickupEnergyBuff", &getIdentity<CONST_MOD_TYPE_ENERGYFROMPICKUPPCT>)
                .addProperty<int, int>("SurvivalistEnergy", &getIdentity<CONST_MOD_TYPE_ENERGYFROMPICKUPPCT>)
                .addProperty<int, int>("TerminalJetSpeed", &getIdentity<CONST_MOD_TYPE_TERMINALJETTINGSPEED>)
                .addProperty<int, int>("RegenTimeBuff", &getIdentity<CONST_MOD_TYPE_TIMETOREGENHEALTHPCT>)
                .addProperty<int, int>("HealthRegenTimeBuff", &getIdentity<CONST_MOD_TYPE_TIMETOREGENHEALTHPCT>)
                .addProperty<int, int>("VehicleCostReduction", &getIdentity<CONST_MOD_TYPE_VEHICLECOSTPCT>)
                .addProperty<int, int>("SkiControlVariance", &getIdentity<CONST_MOD_TYPE_SKICONTROLSIGMASQUARE>)
                .addProperty<int, int>("MeleeDamageReduction", &getIdentity<CONST_MOD_TYPE_RECEIVEMELEEDAMAGEPCT>)
                .addProperty<int, int>("CloseCombat", &getIdentity<CONST_MOD_TYPE_RECEIVEMELEEDAMAGEPCT>)
                .addProperty<int, int>("MeleeCausesFlagDrop", &getIdentity<CONST_MOD_TYPE_VICTIMDROPFLAGONMELEE>)
                .addProperty<int, int>("SonicPunchFlagDrop", &getIdentity<CONST_MOD_TYPE_VICTIMDROPFLAGONMELEE>)
                .addProperty<int, int>("ConcussiveMelee", &getIdentity<CONST_MOD_TYPE_VICTIMDROPFLAGONMELEE>)
                .addProperty<int, int>("PrimaryRangeBuff", &getIdentity<CONST_MOD_TYPE_PRIMARYWEAPONRANGEPCT>)
                .addProperty<int, int>("BeltDamageRadiusBuff", &getIdentity<CONST_MOD_TYPE_OFFHANDDAMAGERADIUSPCT>)
                .addProperty<int, int>("TurretTargetAcquisitionBuff", &getIdentity<CONST_MOD_TYPE_TURRETACQUIRETARGETPCT>)
                .addProperty<int, int>("BlackoutLengthReduction", &getIdentity<CONST_MOD_TYPE_WHITEOUTINTERPSPEEDPCT>)
                .addProperty<int, int>("BackstabMeleeBuff", &getIdentity<CONST_MOD_TYPE_BACKSTABMELEEDAMAGEPCT>)
                .addProperty<int, int>("StoppingDistanceBuff", &getIdentity<CONST_MOD_TYPE_MAXSTOPPINGDISTANCEPCT>)
                .addProperty<int, int>("SecondaryRangeBuff", &getIdentity<CONST_MOD_TYPE_SECONDARYWEAPONRANGEPCT>)
                .addProperty<int, int>("BeltPickupBuff", &getIdentity<CONST_MOD_TYPE_EXTRAGRENADESFROMPICKUP>)
                .addProperty<int, int>("ShieldPackBuff", &getIdentity<CONST_MOD_TYPE_SHIELDPACKEFFECTIVENESS>)
                .addProperty<int, int>("StealthPackEntryTimeReduction", &getIdentity<CONST_MOD_TYPE_STEALTHPACKPULSETIMEPCT>)
                .addProperty<int, int>("PrimaryClipSizeBuff", &getIdentity<CONST_MOD_TYPE_PRIMARYINCREASEDCLIPSIZE>)
                .addProperty<int, int>("BeltKillCreditsBuff", &getIdentity<CONST_MOD_TYPE_EXTRACREDITSFROMBELTKILLS>)
                .addProperty<int, int>("TurretArmorPenetrationBuff", &getIdentity<CONST_MOD_TYPE_TURRETARMORPENETRATIONPCT>)
                .addProperty<int, int>("SecondaryClipSizeBuff", &getIdentity<CONST_MOD_TYPE_SECONDARYINCREASEDCLIPSIZE>)
                .addProperty<int, int>("BeltArmorPenetrationBuff", &getIdentity<CONST_MOD_TYPE_OFFHANDARMORPENETRATIONPCT>)
                .addProperty<int, int>("PrimaryArmorPenetrationBuff", &getIdentity<CONST_MOD_TYPE_PRIMARYARMORPENETRATIONPCT>)
                .addProperty<int, int>("RunoverDamageReduction", &getIdentity<CONST_MOD_TYPE_RUNOVERDAMAGEPROTECTIONPCT>)
                .addProperty<int, int>("SafeFallRunover", &getIdentity<CONST_MOD_TYPE_RUNOVERDAMAGEPROTECTIONPCT>)
                .addProperty<int, int>("PrimaryEnergyCostReduction", &getIdentity<CONST_MOD_TYPE_PRIMARYWEAPONENERGYCOSTPCT>)
                .addProperty<int, int>("AcquisitionTimeByEnemyTurretsBuff", &getIdentity<CONST_MOD_TYPE_ACQUIRETIMEBYENEMYTURRETPCT>)
                .addProperty<int, int>("StealthyTurrets", &getIdentity<CONST_MOD_TYPE_ACQUIRETIMEBYENEMYTURRETPCT>)
                .addProperty<int, int>("SecondaryArmorPenetrationBuff", &getIdentity<CONST_MOD_TYPE_SECONDARYARMORPENETRATIONPCT>)
                .addProperty<int, int>("VehicleEnergyBuff", &getIdentity<CONST_MOD_TYPE_VEHICLEEXTRAENERGY>)
                .addProperty<int, int>("ReachOnPickups", &getIdentity<CONST_MOD_TYPE_AMMOPICKUPREACH>)
                .addProperty<int, int>("VehiclePassengerDamageReductionBuff", &getIdentity<CONST_MOD_TYPE_VEHICLEPASSENGERDMGPROTPCT>)
                .addProperty<int, int>("RepairToolDamagesEnemyObjectives", &getIdentity<CONST_MOD_TYPE_DAMAGEREPAIRENEMYOBJECTIVES>)
                .addProperty<int, int>("PotentialEnergyOnFallDamage", &getIdentity<CONST_MOD_TYPE_POTENTIALENERGYFALLDAMAGE>)
                .addProperty<int, int>("BeltThrowSpeedBuff", &getIdentity<CONST_MOD_TYPE_FASTERTHROWBELTBUFFPCT>)
                .addProperty<int, int>("QuickDrawBelt", &getIdentity<CONST_MOD_TYPE_FASTERTHROWBELTBUFFPCT>)
                .addProperty<int, int>("IgnoreGrenadeEffectsOnSelf", &getIdentity<CONST_MOD_TYPE_IGNOREGRENADESECONDARYONSELF>)
                .addProperty<int, int>("PotentialEnergyDamageTransferBuff", &getIdentity<CONST_MOD_TYPE_POTENTIALENERGYDMGTRANSFERPCT>)
                .addProperty<int, int>("ReachTier", &getIdentity<CONST_MOD_TYPE_FLAGREACHTIER>)
                .addProperty<int, int>("SonicPunch", &getIdentity<CONST_MOD_TYPE_SONICPUNCH>)
                .addProperty<int, int>("SonicPunchRange", &getIdentity<CONST_MOD_TYPE_SONICPUNCHDIST>)
                .addProperty<int, int>("SonicPunchKnockback", &getIdentity<CONST_MOD_TYPE_SONICPUNCHKNOCKBACK>)
                .addProperty<int, int>("Rage", &getIdentity<CONST_MOD_TYPE_RAGE>)
                .addProperty<int, int>("RageEnergyRegen", &getIdentity<CONST_MOD_TYPE_RAGEENERGYREGEN>)
                .addProperty<int, int>("RageTime", &getIdentity<CONST_MOD_TYPE_RAGETIMELENGTH>)
                .addProperty<int, int>("RageHealthRestoration", &getIdentity<CONST_MOD_TYPE_RAGEHEALTHRESTOREPCT>)
                .addProperty<int, int>("StealthPackPulseIgnoreTime", &getIdentity<CONST_MOD_TYPE_IGNOREPULSESTEALTHTIME>)
                .addProperty<int, int>("RageMassReduction", &getIdentity<CONST_MOD_TYPE_RAGEMASSREDUCTION>)
                .addProperty<int, int>("RageMassChange", &getIdentity<CONST_MOD_TYPE_RAGEMASSREDUCTION>)
                .addProperty<int, int>("RageMassBuff", &getIdentity<CONST_MOD_TYPE_RAGEMASSREDUCTION>)
                .addProperty<int, int>("DeployableRepairRateBuff", &getIdentity<CONST_MOD_TYPE_REPAIRDEPLOYABLERATEPCT>)
                .addProperty<int, int>("ShocklanceEnergyCost", &getIdentity<CONST_MOD_TYPE_SHOCKLANCEENERGYCOSTPCT>)
                .addProperty<int, int>("ExtraAmmoSpawnBuff", &getIdentity<CONST_MOD_TYPE_EXTRAAMMOSPAWNPCT>)
            .endNamespace()
            ;
    }
}