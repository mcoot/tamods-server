#include "EventLogger.h"

static FILE* _file = NULL;
static std::string _init_timestamp = "";

namespace EventLogger {

	// Utils
	std::string currentISO8601TimeUTC() {
		auto now = std::chrono::system_clock::now();
		auto itt = std::chrono::system_clock::to_time_t(now);
		std::ostringstream ss;
		ss << std::put_time(gmtime(&itt), "%FT%TZ");
		return ss.str();
	}

	static std::map<Kind, std::string> kindNames = {
		{Kind::GAME_START,		"game_start"},
		{Kind::GAME_END,		"game_end"},
		{Kind::SCORE_CHANGE,	"score_change"},
		{Kind::TAKE_DAMAGE,		"take_damage"},
	};

	std::string getKindName(Kind k) {
		if (kindNames.find(k) == kindNames.end()) return "unknown";
		return kindNames[k];
	}

	// Base event class

	static json getTeamMetadata(ATrGameReplicationInfo* gri, ATrGame* game, int teamIdx) {
		json j = json::object();
		std::string game_mode = Utils::f2std(game->Acronym);
		
		// Team score
		j["score"] = game->Teams[teamIdx]->Score;
		j["num_players"] = gri->GetTeamSize(teamIdx);

		if (game_mode == "TrCTF" || game_mode == "TrCTFBlitz") {
			ATrGame_TRCTF* gameCTF = (ATrGame_TRCTF*)game;
			// Flag status
			switch (Utils::tr_gri->FlagState[teamIdx]) {
			case FLAG_Home:
				j["flag_state"] = "home";
				break;
			case FLAG_HeldFriendly:
			case FLAG_HeldEnemy:
				j["flag_state"] = "held";
				if (gameCTF->m_Flags[teamIdx] && gameCTF->m_Flags[teamIdx]->HolderPRI) {
					j["flag_carrier"] = Utils::f2std(gameCTF->m_Flags[teamIdx]->HolderPRI->PlayerName);
				}
				break;
			default:
				j["flag_state"] = "dropped";
			}
		}

		return j;
	}

	static json getGameMetadata() {
		// Data to add: game mode, scores, game time, player counts
		json j = json::object();

		std::lock_guard<std::mutex> lock(Utils::tr_gri_mutex);
		if (!Utils::tr_gri || !Utils::tr_gri->WorldInfo || !Utils::tr_gri->WorldInfo->Game) {
			// No GRI, null game data
			return json();
		}
		ATrGame* game = (ATrGame*)Utils::tr_gri->WorldInfo->Game;

		// Game mode
		j["game_mode"] = Utils::f2std(game->Acronym);
		// World time (upwards-counting)
		j["world_time"] = Utils::tr_gri->WorldInfo->TimeSeconds;
		// Game timer / overtime status
		j["game_timer"] = Utils::tr_gri->RemainingTime;
		j["overtime"] = (bool)game->bOverTime;
		// Team data
		for (int i = 0; i < 2; ++i) {
			std::string team = i == 1 ? "ds" : "be";
			j[team] = getTeamMetadata(Utils::tr_gri, game, i);
		}

		return j;
	}

	json Event::to_json() {
		return json::object();
	}

	std::string Event::to_string(bool pretty) {
		json j = to_json();
		json meta = json::object();

		// Add metadata about the server and game state
		meta["event_kind"] = getKindName(event_kind);
		meta["srv_start_time"] = _init_timestamp;
		meta["event_time"] = event_timestamp;
		meta["game"] = getGameMetadata();

		j["meta"] = meta;
		return j.dump(pretty ? 2 : -1);
	}

	// Event Types

	json JsonEvent::to_json() {
		return j;
	}

	json TakeDamageEvent::to_json() {
		json j = json::object();

		j["collection_return_point"] = collection_return_point;
		j["victim_name"] = victim_name;
		j["damaging_player_name"] = damaging_player_name;
		j["damaging_actor"] = damaging_actor;
		j["damage_type"] = damage_type;
		j["damaging_actor_dir_type"] = damaging_actor_dir_type;
		j["damage_distance"] = damage_distance;
		j["damage_scale"] = damage_scale;
		j["damage_prescale_amount"] = damage_prescale_amount;
		j["damage_preshield_amount"] = damage_preshield_amount;
		j["damage_unshielded_amount"] = damage_unshielded_amount;
		j["energy_drained"] = energy_drained;
		j["was_rage_impulse"] = was_rage_impulse;
		j["caused_death"] = caused_death;

		return j;
	}

	// Logger functions

	void init() {
		if (!_file) {
			if (_init_timestamp == "") {
				_init_timestamp = currentISO8601TimeUTC();
			}

			const char *profile = getenv("USERPROFILE");
			std::string directory;
			if (profile)
				directory = std::string(profile) + "\\Documents\\";
			else
				directory = std::string("C:\\");

			// Log file name will include timestamp, need to replace colons except for the C:\ one as these are invalid in a filename
			std::string logFilePath = std::string(directory + "tamods_events-" + _init_timestamp + ".log");
			std::replace(logFilePath.begin() + 3, logFilePath.end(), ':', '.');
			//Logger::debug("Log file path: %s", logFilePath.c_str());
			_file = fopen(logFilePath.c_str(), "w+");

			if (!_file) {
				// Failed to open event log file
				Logger::error("Failed to open event log file");
			}
		}
	}

	void cleanup() {
		if (_file) {
			fclose(_file);
		}
	}

	void log(Event& e) {
		if (!g_config.eventLogging) return;
		if (!_file) {
			init();
		}
		if (!_file) return;
		
		std::string output = e.to_string(false);

		fprintf(_file, "%s\n", output.c_str());
		fflush(_file);
	}

	void log_basic(Kind k) {
		log(Event(k));
	}

}