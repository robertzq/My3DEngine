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
    std::string transitionEffect;   // "" = instant；"page_curl" = 贝塞尔翻页
    json transitionParams;
};

// 地图多层加载配置：单个 layer 的 id 与地图文件。
// "map": { "layers": [ {"id":"ground","file":"maps/xx.map"}, ... ] }
struct MapLayerSpec {
    std::string id;
    std::string file;
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
    std::vector<MapLayerSpec> mapLayers;   // 多层地图（新增，可选）
    std::string elevationFile;            // 可选：高度矩阵文件（非必须）
    int elevationStepPx = 0;              // 可选：一级高度屏幕像素步长；0=不使用（无 elevation 场景忽略）
    std::string tileset;
    std::string projectionMode;
    float projectionScale = 1.0f;
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
        data.tileset = j.value("tileset", "");
        data.projectionMode = j.value("projection", "");
        data.projectionScale = j.value("projection_scale", 1.0f);
    // Elevation: 可选高度矩阵文件 + 步长。仅当 scene 示例配置了 elevation 才起作用，
    // 无 elevation 的旧场景（island/sample_demo 等）保持不受影响。
    data.elevationFile = j.value("elevation", "");
    data.elevationStepPx = j.value("elevation_step", 0);
        data.params = j.value("params", json::object());

        // map 字段：兼容旧 string，也支持新 object { "layers": [...] }。
        if (j.contains("map")) {
            const auto& m = j["map"];
            if (m.is_string()) {
                data.map = m.get<std::string>();
            } else if (m.is_object() && m.contains("layers") && m["layers"].is_array()) {
                for (const auto& layer : m["layers"]) {
                    if (!layer.is_object()) continue;
                    MapLayerSpec spec;
                    spec.id = layer.value("id", "");
                    spec.file = layer.value("file", "");
                    if (!spec.file.empty()) data.mapLayers.push_back(spec);
                }
            }
        }

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
                if (t.contains("transition")) {
                    if (t["transition"].is_string()) {
                        transition.transitionEffect = t["transition"].get<std::string>();
                    } else if (t["transition"].is_object()) {
                        transition.transitionEffect = t["transition"].value("type", "");
                        transition.transitionParams = t["transition"];
                    }
                }
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