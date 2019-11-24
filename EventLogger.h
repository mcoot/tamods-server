#pragma once

#include "buildconfig.h"

#include <cstdio>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <mutex>
#include <map>

#include <nlohmann/json.hpp>

#include "Utils.h"
#include "Config.h"

using json = nlohmann::json;

// This is a second logger functionality
// Kind of redundant but rather than being for debugging,
// it's for logging events from a 'production' server
// It logs to json, with the intent that the data be analysed later

namespace EventLogger {

    std::string currentISO8601TimeUTC();

    enum class Kind {
        UNKNOWN = 0,
        GAME_START,
        GAME_END,
        SCORE_CHANGE,
        TAKE_DAMAGE,
    };

    std::string getKindName(Kind k);

    class Event {
    protected:
        Kind event_kind;
        std::string event_timestamp;
    public:
        Event(Kind event_kind) : event_kind(event_kind), event_timestamp(currentISO8601TimeUTC()) {}
        virtual json to_json();

        std::string to_string(bool pretty);
    };

    class JsonEvent : public Event {
    private:
        json j;
    public:
        JsonEvent(json j) : Event(Kind::UNKNOWN), j(j) {}
        JsonEvent(Kind k, json j) : Event(k), j(j) {}
        json to_json() override;
    };

    class TakeDamageEvent : public Event {
    public:
        int collection_return_point = -1;
        std::string victim_name = "";
        std::string damaging_player_name = "";
        std::string damaging_actor = "";
        std::string damage_type = "";
        int damaging_actor_dir_type = -1;
        float damage_distance = -1;
        float damage_scale = -1;
        float damage_prescale_amount = -1;
        float damage_preshield_amount = -1;
        float damage_unshielded_amount = -1;
        float energy_drained = 0;
        bool was_rage_impulse = false;
        bool caused_death = false;
    public:
        TakeDamageEvent() : Event(Kind::TAKE_DAMAGE) {}
        json to_json() override;
    };

    void init();
    void log(Event& e);
    void log_basic(Kind k);
    void cleanup();
};