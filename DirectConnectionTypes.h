#pragma once

#include <sstream>
#include <map>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <nlohmann/json.hpp>


#include "SdkHeaders.h"
#include "TAServerTypes.h"
#include "Logger.h"

#define DCSRV_MSG_KIND_INVALID 0x0

#define DVSRV_MSG_KIND_PLAYER_CONNECTION 0x10000000

namespace DCServer {

	class Message {
	public:
		virtual uint32_t getMessageKind() {
			return DCSRV_MSG_KIND_INVALID;
		}

		virtual void toJson(json& j) {}
		virtual bool fromJson(const json& j) {
			return false;
		}

		bool isInvalidMessage() {
			return getMessageKind() == DCSRV_MSG_KIND_INVALID;
		}
	};

	class PlayerConnectionMessage : public Message {
	public:
		FUniqueNetId uniquePlayerId;
		std::string protocolVersion;
	public:
		uint32_t getMessageKind() override {
			return DVSRV_MSG_KIND_PLAYER_CONNECTION;
		}

		void toJson(json& j) {
			j["player_unique_id"] = TAServer::netIdToLong(uniquePlayerId);
			j["protocol_version"] = protocolVersion;
		}

		bool fromJson(const json& j) {
			auto& it = j.find("player_unique_id");
			if (it == j.end()) return false;
			uniquePlayerId = TAServer::longToNetId(j["player_unique_id"]);

			it = j.find("protocol_version");
			if (it == j.end()) return false;
			protocolVersion = j["protocol_version"].get<std::string>();

			return true;
		}
	};

}