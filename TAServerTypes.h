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

#define TASRV_MSG_KIND_GAME_2_LAUNCHER_PROTOCOL_VERSION 0x3000
#define TASRV_MSG_KIND_GAME_2_LAUNCHER_TEAMINFO 0x3001
#define TASRV_MSG_KIND_GAME_2_LAUNCHER_SCOREINFO 0x3002
#define TASRV_MSG_KIND_GAME_2_LAUNCHER_MATCHTIME 0x3003
#define TASRV_MSG_KIND_GAME_2_LAUNCHER_MATCHEND 0x3004
#define TASRV_MSG_KIND_GAME_2_LAUNCHER_LOADOUT_REQUEST 0x3005
#define TASRV_MSG_KIND_GAME_2_LAUNCHER_MAPINFO 0x3006
#define TASRV_MSG_KIND_GAME_2_LAUNCHER_SERVERINFO 0x3007

#define TASRV_MSG_KIND_LAUNCHER_2_GAME_LOADOUT_MESSAGE 0x4000
#define TASRV_MSG_KIND_LAUNCHER_2_GAME_NEXT_MAP_MESSAGE 0x4001
#define TASRV_MSG_KIND_LAUNCHER_2_GAME_PINGS_MESSAGE 0x4002
#define TASRV_MSG_KIND_LAUNCHER_2_GAME_INIT_MESSAGE 0x4003
#define TASRV_MSG_KIND_LAUNCHER_2_GAME_PLAYER_INFO 0x4004

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

	struct PersistentContext {
	public:
		bool hasContext = false;
		int nextMapIndex = 0;
		std::string nextMapOverride = "";
	public:
		void toJson(json& j) {
			j["next_map_index"] = nextMapIndex;
			j["next_map_override"] = nextMapOverride;
		}

		bool fromJson(const json& j) {
			if (j.find("next_map_index") == j.end()) return false;
			nextMapIndex = j["next_map_index"];
			nextMapOverride = j["next_map_override"].get<std::string>();
			return true;
		}
	};

	struct PlayerXpRecord {
	public:
		int xp = 0;
		bool wasFirstWin = false;
	public:
		void toJson(json& j) {
			j["xp"] = xp;
			j["first_win"] = wasFirstWin;
		}

		bool fromJson(const json& j) {
			if (j.find("xp") == j.end()) return false;
			xp = j["xp"];
			if (j.find("first_win") == j.end()) return false;
			wasFirstWin = j["first_win"];
			return true;
		}
	};

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
			j["player_unique_id"] = Utils::netIdToLong(uniquePlayerId);
			j["class_id"] = classId;
			j["loadout_number"] = slot;
		}

		bool fromJson(const json& j) {
			auto& it = j.find("player_unique_id");
			if (it == j.end()) return false;
			uniquePlayerId = Utils::longToNetId(j["player_unique_id"]);

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
			j["player_unique_id"] = Utils::netIdToLong(uniquePlayerId);
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
			uniquePlayerId = Utils::longToNetId(j["player_unique_id"]);

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

	class Game2LauncherTeamInfoMessage : public Message {
	public:
		std::map<long long, int> playerToTeamId;

	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_GAME_2_LAUNCHER_TEAMINFO;
		}

		void toJson(json& j) {
			json mapping;

			for (auto& elem : playerToTeamId) {
				mapping[std::to_string(elem.first)] = elem.second;
			}

			j["player_to_team_id"] = mapping;
		}

		bool fromJson(const json& j) {
			auto& it = j.find("player_to_team_id");
			if (it == j.end()) return false;
			json mapping = j["player_to_team_id"];

			for (json::iterator mapping_it = mapping.begin(); mapping_it != mapping.end(); ++mapping_it) {
				int teamId = mapping_it.value();

				long long playerIdLong;
				try {
					playerIdLong = std::stoll(mapping_it.key());
				}
				catch (std::invalid_argument&) {
					return false;
				}

				// Must be a valid team
				if (teamId != 0 && teamId != 1 && teamId != 255) return false;

				playerToTeamId[playerIdLong] = teamId;
			}
			return true;
		}
	};

	class Game2LauncherScoreInfoMessage : public Message {
	public:
		int beScore;
		int dsScore;
	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_GAME_2_LAUNCHER_SCOREINFO;
		}

		void toJson(json& j) {
			j["be_score"] = beScore;
			j["ds_score"] = dsScore;
		}

		bool fromJson(const json& j) {
			beScore = j["be_score"];
			dsScore = j["ds_score"];
			return true;
		}
	};

	class Game2LauncherMatchTimeMessage : public Message {
	public:
		long long secondsRemaining;
		bool counting;
	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_GAME_2_LAUNCHER_MATCHTIME;
		}

		void toJson(json& j) {
			j["seconds_remaining"] = secondsRemaining;
			j["counting"] = counting;
		}

		bool fromJson(const json& j) {
			secondsRemaining = j["seconds_remaining"];
			counting = j["counting"];
			return true;
		}
	};

	class Game2LauncherMatchEndMessage : public Message {
	public:
		PersistentContext controllerContext;
		std::map<long long, PlayerXpRecord> playerEarnedXps;
		int nextMapWaitTime;
	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_GAME_2_LAUNCHER_MATCHEND;
		}

		void toJson(json& j) {
			json contextJson;

			controllerContext.toJson(contextJson);

			j["controller_context"] = contextJson;

			json xpJson = json::object();

			for (auto& it : playerEarnedXps) {
				json recJson;
				it.second.toJson(recJson);
				xpJson[std::to_string(it.first)] = recJson;
			}

			j["player_earned_xps"] = xpJson;
			j["next_map_wait_time"] = nextMapWaitTime;
		}

		bool fromJson(const json& j) {
			json mapping = j["player_earned_xps"];
			for (json::iterator mapping_it = mapping.begin(); mapping_it != mapping.end(); ++mapping_it) {
				std::string key = mapping_it.key();
				long long playerIdLong;
				try {
					playerIdLong = std::stoll(key);
				}
				catch (std::invalid_argument&) {
					return false;
				}

				json jsonRec = *mapping_it;
				PlayerXpRecord rec;
				if (!rec.fromJson(jsonRec)) {
					return false;
				}

				playerEarnedXps[playerIdLong] = rec;
			}

			if (j.find("next_map_wait_time") == j.end()) return false;
			nextMapWaitTime = j["next_map_wait_time"];

			if (j.find("controller_context") == j.end()) return false;
			json contextJson = j["controller_context"];

			bool succeeded = controllerContext.fromJson(contextJson);
			if (succeeded) {
				controllerContext.hasContext = true;
			}

			return succeeded;
		}
	};

	class Launcher2GameNextMapMessage : public Message {
	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_LAUNCHER_2_GAME_NEXT_MAP_MESSAGE;
		}

		void toJson(json& j) {}

		bool fromJson(const json& j) {
			return true;
		}
	};

	class Game2LauncherProtocolVersionMessage : public Message {
	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_GAME_2_LAUNCHER_PROTOCOL_VERSION;
		}

		void toJson(json& j) {
			j["version"] = TASERVER_PROTOCOL_VERSION;
		}

		bool fromJson(const json& j) {
			return true;
		}
	};

	class Launcher2GamePingsMessage : public Message {
	public:
		std::map<long long, int> playerToPing;
	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_LAUNCHER_2_GAME_PINGS_MESSAGE;
		}

		void toJson(json& j) {
			json mapping;

			for (auto& elem : playerToPing) {
				mapping[std::to_string(elem.first)] = elem.second;
			}

			j["player_pings"] = mapping;
		}

		bool fromJson(const json& j) {
			auto& it = j.find("player_pings");
			if (it == j.end()) return false;
			json mapping = j["player_pings"];

			for (json::iterator mapping_it = mapping.begin(); mapping_it != mapping.end(); ++mapping_it) {
				std::string key = mapping_it.key();
				long long playerIdLong;
				try {
					playerIdLong = std::stoll(key);
				}
				catch (std::invalid_argument&) {
					return false;
				}

				int pingValue = (*mapping_it).get<int>();

				playerToPing[playerIdLong] = pingValue;
			}
			return true;
		}
	};

	class Game2LauncherMapInfoMessage : public Message {
	public:
		int mapId;
	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_GAME_2_LAUNCHER_MAPINFO;
		}

		void toJson(json& j) {
			j["map_id"] = mapId;
		}

		bool fromJson(const json& j) {
			auto& it = j.find("player_pings");
			if (it == j.end()) return false;
			mapId = j["player_pings"];

			return true;
		}
	};

	class Launcher2GameInitMessage : public Message {
	public:
		PersistentContext controllerContext;

	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_LAUNCHER_2_GAME_INIT_MESSAGE;
		}

		void toJson(json& j) {
			json contextJson;
			
			controllerContext.toJson(contextJson);

			j["controller_context"] = contextJson;
		}

		bool fromJson(const json& j) {
			if (j.find("controller_context") == j.end()) return false;
			json contextJson = j["controller_context"];

			if (!contextJson.is_object()) return false;
			if (contextJson.empty()) {
				// Empty context
				// Fine, we treat the server as just starting for the first time
				controllerContext.hasContext = false;
				return true;
			}

			bool succeeded = controllerContext.fromJson(contextJson);
			if (succeeded) {
				controllerContext.hasContext = true;
			}

			return succeeded;
		}
	};

	class Launcher2GamePlayerInfoMessage : public Message {
	public:
		long long playerId;
		int rankXp;
		bool eligibleForFirstWin;
	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_LAUNCHER_2_GAME_PLAYER_INFO;
		}

		void toJson(json& j) {
			j["player_unique_id"] = playerId;
			j["rank_xp"] = rankXp;
			j["eligible_for_first_win"] = eligibleForFirstWin;
		}

		bool fromJson(const json& j) {
			if (j.find("player_unique_id") == j.end()) return false;
			playerId = j["player_unique_id"];

			if (j.find("rank_xp") == j.end()) return false;
			rankXp = j["rank_xp"];

			if (j.find("eligible_for_first_win") == j.end()) return false;
			eligibleForFirstWin = j["eligible_for_first_win"];

			return true;
		}
	};

	class Game2LauncherServerInfoMessage : public Message {
	public:
		std::string description;
		std::string motd;
		std::vector<unsigned char> password_hash;
		std::string game_setting_mode;
	public:
		short getMessageKind() override {
			return TASRV_MSG_KIND_GAME_2_LAUNCHER_SERVERINFO;
		}

		void toJson(json& j) {
			j["description"] = description;
			j["motd"] = motd;
			if (password_hash.empty()) {
				j["password_hash"] = json();
			} 
			else {
				j["password_hash"] = password_hash;
			}
			j["game_setting_mode"] = game_setting_mode;
		}

		bool fromJson(const json& j) {
			// Not needed
			return false;
		}
	};
}