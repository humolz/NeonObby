#pragma once

#include "ecs/World.h"
#include "level/ObstacleFactory.h"
#include <nlohmann/json.hpp>
#include <string>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

struct LevelData {
    std::string name;
    glm::vec3 spawnPoint{0, 3, 0};
    bool loaded = false;
};

class LevelLoader {
public:
    static LevelData load(const std::string& filepath, World& world) {
        LevelData data;

        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open level: " << filepath << "\n";
            return data;
        }

        json j;
        try {
            file >> j;
        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << "\n";
            return data;
        }

        data.name = j.value("name", "Unnamed Level");
        if (j.contains("spawnPoint")) {
            data.spawnPoint = readVec3(j["spawnPoint"]);
        }

        // Load obstacles
        if (j.contains("obstacles")) {
            for (auto& obj : j["obstacles"]) {
                loadObstacle(obj, world);
            }
        }

        // Load triggers
        if (j.contains("triggers")) {
            for (auto& obj : j["triggers"]) {
                loadTrigger(obj, world);
            }
        }

        data.loaded = true;
        return data;
    }

private:
    static glm::vec3 readVec3(const json& j) {
        return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
    }

    static void loadObstacle(const json& obj, World& world) {
        std::string type = obj.value("type", "StaticPlatform");
        glm::vec3 pos = readVec3(obj["position"]);
        glm::vec3 scale = readVec3(obj["scale"]);
        glm::vec3 baseColor = readVec3(obj["baseColor"]);
        glm::vec3 emissionColor = readVec3(obj["emissionColor"]);
        float emissionStrength = obj.value("emissionStrength", 0.5f);

        if (type == "StaticPlatform") {
            ObstacleFactory::createPlatform(world, pos, scale, baseColor, emissionColor, emissionStrength);
        }
        else if (type == "MovingPlatform") {
            std::vector<glm::vec3> waypoints;
            for (auto& wp : obj["waypoints"]) {
                waypoints.push_back(readVec3(wp));
            }
            float speed = obj.value("speed", 3.0f);
            float pauseTime = obj.value("pauseTime", 0.5f);
            ObstacleFactory::createMovingPlatform(world, pos, scale, baseColor, emissionColor,
                                                   emissionStrength, waypoints, speed, pauseTime);
        }
        else if (type == "RotatingPlatform") {
            glm::vec3 rotAxis = obj.contains("rotationAxis") ? readVec3(obj["rotationAxis"]) : glm::vec3(0, 1, 0);
            float rotSpeed = obj.value("rotationSpeed", 45.0f);
            ObstacleFactory::createRotatingPlatform(world, pos, scale, baseColor, emissionColor,
                                                     emissionStrength, rotAxis, rotSpeed);
        }
        else if (type == "Spinner") {
            float rotSpeed = obj.value("rotationSpeed", 90.0f);
            ObstacleFactory::createSpinner(world, pos, scale, baseColor, emissionColor,
                                            emissionStrength, rotSpeed);
        }
        else if (type == "TimedObstacle") {
            float onDur = obj.value("onDuration", 2.0f);
            float offDur = obj.value("offDuration", 1.5f);
            float phase = obj.value("phase", 0.0f);
            ObstacleFactory::createTimedPlatform(world, pos, scale, baseColor, emissionColor,
                                                  emissionStrength, onDur, offDur, phase);
        }
        else if (type == "LowWall") {
            ObstacleFactory::createLowWall(world, pos, scale, baseColor, emissionColor, emissionStrength);
        }
        else if (type == "CrawlTunnel") {
            ObstacleFactory::createCrawlTunnel(world, pos, scale, baseColor, emissionColor, emissionStrength);
        }
    }

    static void loadTrigger(const json& obj, World& world) {
        std::string type = obj.value("type", "KillZone");
        glm::vec3 pos = readVec3(obj["position"]);
        glm::vec3 scale = readVec3(obj["scale"]);

        if (type == "KillZone") {
            ObstacleFactory::createKillZone(world, pos, scale);
        }
        else if (type == "JumpPad") {
            float strength = obj.value("strength", 15.0f);
            ObstacleFactory::createJumpPad(world, pos, scale, strength);
        }
        else if (type == "SpeedBoost") {
            float strength = obj.value("strength", 20.0f);
            ObstacleFactory::createSpeedBoost(world, pos, scale, strength);
        }
        else if (type == "Checkpoint") {
            int index = obj.value("checkpointIndex", 0);
            glm::vec3 respawn = obj.contains("respawnPosition") ? readVec3(obj["respawnPosition"]) : pos + glm::vec3(0, 2, 0);
            ObstacleFactory::createCheckpoint(world, pos, index, respawn);
        }
        else if (type == "LevelFinish") {
            ObstacleFactory::createFinishLine(world, pos, scale);
        }
    }
};
