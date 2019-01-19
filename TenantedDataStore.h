#pragma once

#include <sstream>
#include <list>
#include <random>

#include "FunctionalUtils.h"

#include "TAServerClient.h"
#include "DirectConnectionServer.h"

#include "Config.h"
#include "Utils.h"
#include "Data.h"
#include "Geom.h"
#include "Logger.h"
#include "LuaAPI.h"

/**
 * The TenantedDataStore is used to get around not being able to add fields to in-game classes
 * So for instance, data that needs to be stored per-player will be stored here
 *
*/

namespace TenantedDataStore {

	struct CallInData {
		float lastInvCallInTime = -1;
		float invCallInEndTime = -1;
		FVector lastTargetPos = FVector(0, 0, 0);
	};

	struct PlayerSpecificData {
		long long playerId = -1;

		// Data used for call-ins
		CallInData callInData;

		// Data used to determine assists and kill credits
		std::vector<FAssistInfo> assistInfo;

		// Jackal rounds currently out
		std::vector<ATrProj_RemoteArxBuster*> remoteArxRounds;
	};

	// A thread-safe map which returns the default constructed value if a key isn't found
	template <typename KeyType, typename ValueType>
	class DataMap {
	private:
		std::mutex storeMutex;
		std::map<KeyType, ValueType > store;
	public:
		ValueType get(KeyType k) {
			std::lock_guard<decltype(storeMutex)> lock(storeMutex);
			if (store.find(k) == store.end()) return ValueType();

			return store[k];
		}

		void set(KeyType k, ValueType v) {
			std::lock_guard<decltype(storeMutex)> lock(storeMutex);
			store[k] = v;
		}
	};

	
	extern DataMap<long long, PlayerSpecificData> playerData;

}