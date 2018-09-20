#include "Mods.h"

namespace GameBalance {
	namespace Items {

		bool Property::apply(PropValue value, ATrDevice* dev) {
			return (value.type == type && applier(value, dev));
		}

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

static void setWeaponProp(std::string className, std::string itemName, int propId, LuaRef val) {
	int itemId = Data::getItemId(className, itemName);

	// TODO: Check propId is real

	// TODO: check val is the right type

	// TODO: create map for item if there isn't one, put prop reference in
}

namespace LuaAPI {
	void addGameBalanceAPI(luabridge::Namespace ns) {
		ns
			.beginNamespace("GameBalance")
				.addFunction("SetItemProperty", &setWeaponProp)
			.endNamespace();
	}
}