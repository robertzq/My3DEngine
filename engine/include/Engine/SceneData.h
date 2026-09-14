#pragma once
#include <SDL.h>
#include <map>
#include <string>
#include <vector>
#include "Engine/json.hpp"

using json = nlohmann::json;

struct SpawnPoint {
    int x = 0;
    int y = 0;
};

struct EntityDef {
    std::string id;
    std::string behavior;
    std::string tag;
    std::string texture;
    std::string shader;          // optional ShaderManager id
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    bool visible = true;
    bool hasCollider = false;
    SDL_Rect src{0, 0, 0, 0};
    SDL_Rect collider{0, 0, 0, 0};
    json params;
    json shaderParams;           // optional { uniform: value }
};

struct SceneTransition {
    std::string trigger;
    std::string target;
    std::string spawn = "default";
    bool automatic = true;
    json params;
};

namespace scene_data_detail {

inline EntityDef ParseEntity(const json& j) {
    EntityDef def;
    def.id = j.value("id", "");
    def.behavior = j.value("behavior", "");
    def.tag = j.value("tag", "");
    def.texture = j.value("texture", "");
    def.shader = j.value("shader", "");
    def.visible = j.value("visible", true);
    def.params = j.value("params", json::object());
    if (j.contains("shader_params") && j["shader_params"].is_object()) {
        def.shaderParams = j["shader_params"];
    }

    if (j.contains("at") && j["at"].is_array() && j["at"].size() >= 2) {
        def.x = j["at"][0].get<int>();
        def.y = j["at"][1].get<int>();
    }
    if (j.contains("size") && j["size"].is_array() && j["size"].size() >= 2) {
        def.w = j["size"][0].get<int>();
        def.h = j["size"][1].get<int>();
    }
    if (j.contains("src") && j["src"].is_array() && j["src"].size() >= 4) {
        def.src.x = j["src"][0].get<int>();
        def.src.y = j["src"][1].get<int>();
        def.src.w = j["src"][2].get<int>();
        def.src.h = j["src"][3].get<int>();
    }
    if (j.contains("collider") && j["collider"].is_array() && j["collider"].size() >= 4) {
        def.collider.x = j["collider"][0].get<int>();
        def.collider.y = j["collider"][1].get<int>();
        def.collider.w = j["collider"][2].get<int>();
        def.collider.h = j["collider"][3].get<int>();
        def.hasCollider = true;
    }
    return def;
}

}

struct SceneData {
    std::string id;
    std::string view;
    std::string controller;
    std::string map;
    std::string tileset;
    std::map<std::string, SpawnPoint> spawns;
    std::vector<SceneTransition> transitions;
    std::vector<EntityDef> entities;
    bool hasPlayer = false;
    EntityDef player;
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

        if (j.contains("player") && j["player"].is_object()) {
            data.player = scene_data_detail::ParseEntity(j["player"]);
            data.hasPlayer = true;
        }

        if (j.contains("entities") && j["entities"].is_array()) {
            for (const auto& e : j["entities"]) {
                if (e.is_object()) data.entities.push_back(scene_data_detail::ParseEntity(e));
            }
        }

        return data;
    }
};
