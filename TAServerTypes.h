#pragma once

#include <sstream>
#include <map>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <nlohmann/json.hpp>


#include "SdkHeaders.h"
#include "Logger.h"

using json = nlohmann::json;

#define TASRV_MSG_KIND_INVALID 0x0
#define TASRV_MSG_KIND_GAME_2_LAUNCHER_LOADOUT_REQUEST 0x3002
#define TASRV_MSG_KIND_LAUNCHER_2_GAME_LOADOUT_MESSAGE 0x4000

namespace TAServer {

	class Message {
	public:
		virtual short getMessageKind() {
			return TASRV_MSG_KIND_INVALID;
		}

		virtual void toJson(json& j) {}
		virtual bool fromJson(const json& j) {
			return false;
		}

		bool isInvalidMessage() {
			return getMessageKind() == TASRV_MSG_KIND_INVALID;
		}
	};

	class Game2LauncherLoadoutRequest : public Message {
	public:
		FUniqueNetId uniquePlayerId;
		int classId;
		int slot;

	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_GAME_2_LAUNCHER_LOADOUT_REQUEST;
		}

		void toJson(json& j) {
			long long tmpA = uniquePlayerId.Uid.A;
			unsigned long long uniqueIdNum = (tmpA << 32) | (uniquePlayerId.Uid.B);
			j["player_unique_id"] = uniqueIdNum;
			j["class_id"] = classId;
			j["loadout_number"] = slot;
		}

		bool fromJson(const json& j) {
			auto& it = j.find("player_unique_id");
			if (it == j.end()) return false;
			long long rawUId = j["player_unique_id"];
			uniquePlayerId.Uid.A = rawUId >> 32;
			uniquePlayerId.Uid.B = rawUId & 0x00000000FFFFFFFF;

			it = j.find("class_id");
			if (it == j.end()) return false;
			classId = j["class_id"];

			it = j.find("loadout_number");
			if (it == j.end()) return false;
			slot = j["loadout_number"];

			return true;
		}
	};

	class Launcher2GameLoadoutMessage : public Message {
	private:
		const std::string CODE_LOADOUTNAME = "1341";
		const std::string CODE_PRIMARY = "1086";
		const std::string CODE_SECONDARY = "1087";
		const std::string CODE_TERTIARY = "1765";
		const std::string CODE_BELT = "1089";
		const std::string CODE_PACK = "1088";
		const std::string CODE_PERKA = "1090";
		const std::string CODE_PERKB = "1091";
		const std::string CODE_SKIN = "1093";
		const std::string CODE_VOICE = "1094";
	public:
		std::string loadoutName = "";
		int primary = 0;
		int secondary = 0;
		int tertiary = 0;
		int belt = 0;
		int pack = 0;
		int perkA = 0;
		int perkB = 0;
		int skin = 0;
		int voice = 0;
	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_LAUNCHER_2_GAME_LOADOUT_MESSAGE;
		}

		void toJson(json& j) {
			j["loadout"] = {
				{CODE_LOADOUTNAME, loadoutName},
				{CODE_PRIMARY, primary},
				{CODE_SECONDARY, secondary},
				{CODE_TERTIARY, tertiary},
				{CODE_BELT, belt},
				{CODE_PACK, pack},
				{CODE_PERKA, perkA},
				{CODE_PERKB, perkB},
				{CODE_SKIN, skin},
				{CODE_VOICE, voice}
			};
		}

		bool fromJson(const json& j) {
			auto& it = j.find("loadout");
			if (it == j.end()) return false;
			json jl = j["loadout"];

			it = jl.find(CODE_LOADOUTNAME);
			if (it != jl.end()) {
				loadoutName = jl[CODE_LOADOUTNAME].get<std::string>();
			}
			it = jl.find(CODE_PRIMARY);
			if (it != jl.end()) {
				primary = jl[CODE_PRIMARY];
			}
			it = jl.find(CODE_SECONDARY);
			if (it != jl.end()) {
				secondary = jl[CODE_SECONDARY];
			}
			it = jl.find(CODE_TERTIARY);
			if (it != jl.end()) {
				tertiary = jl[CODE_TERTIARY];
			}
			it = jl.find(CODE_BELT);
			if (it != jl.end()) {
				belt = jl[CODE_BELT];
			}
			it = jl.find(CODE_PACK);
			if (it != jl.end()) {
				pack = jl[CODE_PACK];
			}
			it = jl.find(CODE_PERKA);
			if (it != jl.end()) {
				perkA = jl[CODE_PERKA];
			}
			it = jl.find(CODE_PERKB);
			if (it != jl.end()) {
				perkB = jl[CODE_PERKB];
			}
			it = jl.find(CODE_SKIN);
			if (it != jl.end()) {
				skin = jl[CODE_SKIN];
			}
			it = jl.find(CODE_VOICE);
			if (it != jl.end()) {
				voice = jl[CODE_VOICE];
			}

			return true;
		}

		void toEquipPointMap(std::map<int, int>& m) {
			m[EQP_Primary] = primary;
			m[EQP_Secondary] = secondary;
			m[EQP_Tertiary] = tertiary;
			m[EQP_Belt] = belt;
			m[EQP_Pack] = pack;
			m[EQP_PerkA] = perkA;
			m[EQP_PerkB] = perkB;
			m[EQP_Skin] = skin;
			m[EQP_Voice] = voice;
		}
	};

	std::shared_ptr<Message> parseMessageFromJson(short msgKind, const json& j);

}