#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#endif

using json = nlohmann::json;

struct SaveData {
    std::vector<bool> levelsCompleted;   // which levels have been beaten
    std::vector<float> bestTimes;        // best time per level
    std::vector<int> bestDeaths;         // fewest deaths per level

    bool isUnlocked(int levelIndex) const {
        if (levelIndex == 0) return true; // Level 1 always unlocked
        if (levelIndex - 1 < static_cast<int>(levelsCompleted.size())) {
            return levelsCompleted[levelIndex - 1];
        }
        return false;
    }
};

class SaveManager {
public:
    void init(int totalLevels) {
        m_totalLevels = totalLevels;
        m_savePath = getSavePath();
        m_data.levelsCompleted.resize(totalLevels, false);
        m_data.bestTimes.resize(totalLevels, 0.0f);
        m_data.bestDeaths.resize(totalLevels, -1);
        load();
    }

    const SaveData& data() const { return m_data; }

    void onLevelComplete(int levelIndex, float time, int deaths) {
        if (levelIndex < 0 || levelIndex >= m_totalLevels) return;

        m_data.levelsCompleted[levelIndex] = true;

        if (m_data.bestTimes[levelIndex] <= 0.0f || time < m_data.bestTimes[levelIndex]) {
            m_data.bestTimes[levelIndex] = time;
        }

        if (m_data.bestDeaths[levelIndex] < 0 || deaths < m_data.bestDeaths[levelIndex]) {
            m_data.bestDeaths[levelIndex] = deaths;
        }

        save();
    }

private:
    SaveData m_data;
    std::string m_savePath;
    int m_totalLevels = 0;

    std::string getSavePath() {
        std::string dir;
#ifdef _WIN32
        char path[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path) == S_OK) {
            dir = std::string(path) + "\\NeonObby";
        } else {
            dir = ".";
        }
        // Create directory if it doesn't exist
        CreateDirectoryA(dir.c_str(), NULL);
#else
        const char* home = std::getenv("HOME");
        dir = home ? std::string(home) + "/.neonobby" : ".";
        // mkdir -p equivalent
        std::string cmd = "mkdir -p " + dir;
        (void)system(cmd.c_str());
#endif
        return dir + "/save.json";
    }

    void load() {
        std::ifstream file(m_savePath);
        if (!file.is_open()) return;

        try {
            json j;
            file >> j;

            if (j.contains("levelsCompleted")) {
                auto& arr = j["levelsCompleted"];
                for (int i = 0; i < m_totalLevels && i < static_cast<int>(arr.size()); i++) {
                    m_data.levelsCompleted[i] = arr[i].get<bool>();
                }
            }
            if (j.contains("bestTimes")) {
                auto& arr = j["bestTimes"];
                for (int i = 0; i < m_totalLevels && i < static_cast<int>(arr.size()); i++) {
                    m_data.bestTimes[i] = arr[i].get<float>();
                }
            }
            if (j.contains("bestDeaths")) {
                auto& arr = j["bestDeaths"];
                for (int i = 0; i < m_totalLevels && i < static_cast<int>(arr.size()); i++) {
                    m_data.bestDeaths[i] = arr[i].get<int>();
                }
            }
        } catch (...) {
            std::cerr << "Failed to parse save file\n";
        }
    }

    void save() {
        json j;
        j["levelsCompleted"] = m_data.levelsCompleted;
        j["bestTimes"] = m_data.bestTimes;
        j["bestDeaths"] = m_data.bestDeaths;

        std::ofstream file(m_savePath);
        if (file.is_open()) {
            file << j.dump(2);
        }
    }
};
