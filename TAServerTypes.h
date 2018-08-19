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
#define TASRV_MSG_KIND_GAME_2_LAUNCHER_LOADOUT_REQUEST 0x3005
#define TASRV_MSG_KIND_LAUNCHER_2_GAME_LOADOUT_MESSAGE 0x4000

#define TASRV_EQP_CODE_LOADOUTNAME "1341"
#define TASRV_EQP_CODE_PRIMARY "1086"
#define TASRV_EQP_CODE_SECONDARY "1087"
#define TASRV_EQP_CODE_TERTIARY "1765"
#define TASRV_EQP_CODE_BELT "1089"
#define TASRV_EQP_CODE_PACK "1088"
#define TASRV_EQP_CODE_PERKA "1090"
#define TASRV_EQP_CODE_PERKB "1091"
#define TASRV_EQP_CODE_SKIN "1093"
#define TASRV_EQP_CODE_VOICE "1094"

namespace TAServer {

	long long netIdToLong(FUniqueNetId id);

	FUniqueNetId longToNetId(long long id);

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
			j["player_unique_id"] = netIdToLong(uniquePlayerId);
			j["class_id"] = classId;
			j["loadout_number"] = slot;
		}

		bool fromJson(const json& j) {
			auto& it = j.find("player_unique_id");
			if (it == j.end()) return false;
			uniquePlayerId = longToNetId(j["player_unique_id"]);

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
	public:
		FUniqueNetId uniquePlayerId = {};
		int classId = 0;
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
			j["player_unique_id"] = netIdToLong(uniquePlayerId);
			j["class_id"] = classId;
			j["loadout"] = {
				{TASRV_EQP_CODE_LOADOUTNAME, loadoutName },
				{ TASRV_EQP_CODE_PRIMARY, primary },
				{ TASRV_EQP_CODE_SECONDARY, secondary },
				{ TASRV_EQP_CODE_TERTIARY, tertiary },
				{ TASRV_EQP_CODE_BELT, belt },
				{ TASRV_EQP_CODE_PACK, pack },
				{ TASRV_EQP_CODE_PERKA, perkA },
				{ TASRV_EQP_CODE_PERKB, perkB },
				{ TASRV_EQP_CODE_SKIN, skin },
				{ TASRV_EQP_CODE_VOICE, voice }
			};
		}

		bool fromJson(const json& j) {
			auto& it = j.find("player_unique_id");
			if (it == j.end()) return false;
			uniquePlayerId = longToNetId(j["player_unique_id"]);

			it = j.find("class_id");
			if (it == j.end()) return false;
			classId = j["class_id"];

			it = j.find("loadout");
			if (it == j.end()) return false;
			json jl = j["loadout"];

			it = jl.find(TASRV_EQP_CODE_LOADOUTNAME);
			if (it != jl.end()) {
				loadoutName = jl[TASRV_EQP_CODE_LOADOUTNAME].get<std::string>();
			}
			it = jl.find(TASRV_EQP_CODE_PRIMARY);
			if (it != jl.end()) {
				primary = jl[TASRV_EQP_CODE_PRIMARY];
			}
			it = jl.find(TASRV_EQP_CODE_SECONDARY);
			if (it != jl.end()) {
				secondary = jl[TASRV_EQP_CODE_SECONDARY];
			}
			it = jl.find(TASRV_EQP_CODE_TERTIARY);
			if (it != jl.end()) {
				tertiary = jl[TASRV_EQP_CODE_TERTIARY];
			}
			it = jl.find(TASRV_EQP_CODE_BELT);
			if (it != jl.end()) {
				belt = jl[TASRV_EQP_CODE_BELT];
			}
			it = jl.find(TASRV_EQP_CODE_PACK);
			if (it != jl.end()) {
				pack = jl[TASRV_EQP_CODE_PACK];
			}
			it = jl.find(TASRV_EQP_CODE_PERKA);
			if (it != jl.end()) {
				perkA = jl[TASRV_EQP_CODE_PERKA];
			}
			it = jl.find(TASRV_EQP_CODE_PERKB);
			if (it != jl.end()) {
				perkB = jl[TASRV_EQP_CODE_PERKB];
			}
			it = jl.find(TASRV_EQP_CODE_SKIN);
			if (it != jl.end()) {
				skin = jl[TASRV_EQP_CODE_SKIN];
			}
			it = jl.find(TASRV_EQP_CODE_VOICE);
			if (it != jl.end()) {
				voice = jl[TASRV_EQP_CODE_VOICE];
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
}