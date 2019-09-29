#pragma once

#include <array>
#include <string>
#include <map>

#include "Utils.h"
#include "Data.h"

struct HardCodedLoadouts {
    std::map<int, std::string> loadouts[3][9];

    HardCodedLoadouts() {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 9; ++j) {
                loadouts[i][j] = std::map<int, std::string>();
                loadouts[i][j].clear();
            }
        }
    }

    std::string get(unsigned classNum, int slot, int eqp) {
        if (classNum >= 3 || slot < 0 || slot > 8 || eqp < 0 || eqp > EQP_MAX) return "";
        
        auto& it = loadouts[classNum][slot].find(eqp);
        if (it == loadouts[classNum][slot].end()) {
            return "";
        }
        else {
            return it->second;
        }
    }

    void set(unsigned classNum, int slot, int eqp, std::string item) {
        if (classNum >= 3 || slot < 0 || slot > 8 || eqp < 0 || eqp > EQP_MAX) return;
        loadouts[classNum][slot][eqp] = item;
    }

    template <unsigned classNum>
    std::string getTemplated(int slot, int eqp) {
        return get(classNum, slot, eqp);
    }

    template <unsigned classNum>
    void setTemplated(int slot, int eqp, std::string item) {
        set(classNum, slot, eqp, item);
    }
};
