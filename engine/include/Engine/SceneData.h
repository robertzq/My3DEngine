#pragma once
#include <map>
#include <string>
#include <vector>
#include "Engine/json.hpp"

using json = nlohmann::json;

struct SpawnPoint {
    int x = 0;
    int y = 0;
};

struct SceneTransition {
    std::string trigger;
    std::string target;
    std::string spawn = "default";
    bool automatic = true;
    json params;
};

struct SceneData {
    std::string id;
    std::string view;
    std::string controller;
    std::string map;
    std::string tileset;
    std::map<std::string, SpawnPoint> spawns;
    std::vector<SceneTransition> transitions;
    json params;

    static SceneData FromJson(const std::string& sceneId, const json& j) {
        SceneData data;
        data.id = sceneId;
        data.view = j.value("view", "");
        data.controller = j.value("controller", "");
        data.map = j.value("map", "");
        data.tileset = j.value("tileset", "");
        data.params = j.value("params", json::object());

        if (j.contains("spawns")) {
            for (auto it = j["spawns"].begin(); it != j["spawns"].end(); ++it) {
                SpawnPoint point;
                if (it.value().is_array() && it.value().size() >= 2) {
                    point.x = it.value()[0].get<int>();
                    point.y = it.value()[1].get<int>();
                } else if (it.value().is_object()) {
                    point.x = it.value().value("x", 0);
                    point.y = it.value().value("y", 0);
                }
                data.spawns[it.key()] = point;
            }
        }

        if (j.contains("transitions")) {
            for (const auto& t : j["transitions"]) {
                SceneTransition transition;
                transition.trigger = t.value("trigger", "");
                transition.target = t.value("target", "");
                transition.spawn = t.value("spawn", "default");
                transition.automatic = t.value("automatic", true);
                transition.params = t.value("params", json::object());
                data.transitions.push_back(transition);
            }
        }

        return data;
    }
};
