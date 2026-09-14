#include "Engine/SceneManager.h"
#include "Engine/Config.h"
#include "Engine/Game.h"
#include "Engine/Log.h"
#include "Engine/Physics.h"
#include "Engine/ResourceManager.h"
#include "Engine/SceneController.h"
#include "Engine/SceneRegistry.h"
#include "Engine/SceneView.h"

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() { ClearScene(); }

bool SceneManager::LoadConfig(const std::string& configResourceId) {
    std::string content = ResourceManager::GetTextContent(configResourceId);
    if (content.empty()) {
        LOG_ERROR("SceneManager: 无法读取配置 -> " << configResourceId);
        return false;
    }

    json config;
    try {
        config = json::parse(content);
    } catch (const std::exception& e) {
        LOG_ERROR("SceneManager: 配置解析失败 -> " << e.what());
        return false;
    }

    if (config.contains("tilesets")) {
        for (auto it = config["tilesets"].begin(); it != config["tilesets"].end(); ++it) {
            TileSet tileSet;
            tileSet.tileSize = it.value().value("tile_size", EngineConfig::TILE_SIZE);
            if (it.value().contains("tiles")) {
                for (auto tile = it.value()["tiles"].begin(); tile != it.value()["tiles"].end(); ++tile) {
                    int id = std::stoi(tile.key());
                    TileDef def;
                    def.texture = tile.value().value("texture", "");
                    def.solid = tile.value().value("solid", false);
                    def.trigger = tile.value().value("trigger", "");
                    tileSet.tiles[id] = def;
                }
            }
            tilesets[it.key()] = tileSet;
        }
    }

    if (config.contains("scenes")) {
        for (auto it = config["scenes"].begin(); it != config["scenes"].end(); ++it) {
            scenes[it.key()] = SceneData::FromJson(it.key(), it.value());
        }
    }

    if (config.contains("initial")) {
        initialScene = config["initial"].value("scene", "");
        initialSpawn = config["initial"].value("spawn", "default");
    } else {
        initialScene = config.value("initial_scene", "");
    }

    LOG_INFO("SceneManager: 已加载 " << scenes.size() << " 个场景, " << tilesets.size() << " 个图块集");
    return true;
}

void SceneManager::Start(const std::string& sceneId, const std::string& spawn) {
    LoadScene(sceneId, spawn, json::object());
}

void SceneManager::Start() {
    if (initialScene.empty()) {
        LOG_ERROR("SceneManager: 配置中没有初始场景");
        return;
    }
    Start(initialScene, initialSpawn);
}

void SceneManager::RequestScene(const std::string& sceneId, const std::string& spawn, const json& runtimeParams) {
    hasPending = true;
    pendingScene = sceneId;
    pendingSpawn = spawn;
    pendingParams = runtimeParams;
}

void SceneManager::RequestTransition(const std::string& trigger, const json& runtimeParams) {
    if (!current) return;
    for (const auto& transition : current->transitions) {
        if (transition.trigger == trigger) {
            json params = transition.params;
            if (runtimeParams.is_object()) {
                for (auto it = runtimeParams.begin(); it != runtimeParams.end(); ++it) params[it.key()] = it.value();
            }
            RequestScene(transition.target, transition.spawn, params);
            return;
        }
    }
    LOG_WARN("SceneManager: 未找到触发器对应的转场 -> " << trigger);
}

void SceneManager::SetPlayer(GameObject* newPlayer) {
    player = newPlayer;
    if (player && map) {
        player->SetWorldWidth(static_cast<float>(map->WidthPx()));
        player->SetWorldHeight(static_cast<float>(map->HeightPx()));
    }
}

SDL_Point SceneManager::SpawnPosition(const std::string& spawn, const json& runtimeParams) const {
    if (runtimeParams.contains("spawn_x") && runtimeParams.contains("spawn_y")) {
        return {runtimeParams["spawn_x"].get<int>(), runtimeParams["spawn_y"].get<int>()};
    }
    if (current) {
        auto it = current->spawns.find(spawn);
        if (it != current->spawns.end()) return {it->second.x, it->second.y};
        auto fallback = current->spawns.find("default");
        if (fallback != current->spawns.end()) return {fallback->second.x, fallback->second.y};
    }
    return {0, 0};
}

void SceneManager::ClearScene() {
    if (view) view->OnExit();
    if (controller) controller->OnExit();
    view.reset();
    controller.reset();
    map.reset();
    player = nullptr;
    current = nullptr;
}

void SceneManager::LoadScene(const std::string& sceneId, const std::string& spawn, const json& runtimeParams) {
    auto it = scenes.find(sceneId);
    if (it == scenes.end()) {
        LOG_ERROR("SceneManager: 未知场景 -> " << sceneId);
        return;
    }
    const SceneData& data = it->second;

    ClearScene();
    current = &data;

    if (!data.map.empty() && !data.tileset.empty()) {
        auto tileSetIt = tilesets.find(data.tileset);
        if (tileSetIt == tilesets.end()) {
            LOG_ERROR("SceneManager: 未知图块集 -> " << data.tileset);
        } else {
            map = std::make_unique<TileMap>();
            map->Load(data.map, tileSetIt->second);
        }
    }

    if (map) {
        Game::camera = {0, 0, EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT};
        Game::cameraX_float = 0.0f;
    }

    context.game = Game::instance();
    context.manager = this;
    context.data = &data;
    context.map = map.get();
    context.controller = nullptr;
    context.params = data.params.is_object() ? data.params : json::object();
    context.params["scene"] = sceneId;
    context.params["spawn"] = spawn;
    if (runtimeParams.is_object()) {
        for (auto p = runtimeParams.begin(); p != runtimeParams.end(); ++p) context.params[p.key()] = p.value();
    }

    if (!data.controller.empty()) {
        controller.reset(SceneRegistry::CreateController(data.controller));
        if (!controller) LOG_WARN("SceneManager: 控制器未注册 -> " << data.controller);
    }
    if (!data.view.empty()) {
        view.reset(SceneRegistry::CreateView(data.view));
        if (!view) LOG_ERROR("SceneManager: 视图未注册 -> " << data.view);
    }

    context.controller = controller.get();
    if (controller) controller->OnEnter(context);
    if (view) view->OnEnter(context);

    if (player && context.params.contains("spawn_x") && context.params.contains("spawn_y")) {
        player->SetPos(context.params["spawn_x"].get<float>(), context.params["spawn_y"].get<float>());
    }

    LOG_INFO("SceneManager: 进入场景 " << sceneId << " (spawn=" << spawn << ")");
}

std::vector<SDL_Rect> SceneManager::Colliders() const {
    return map ? map->Colliders() : std::vector<SDL_Rect>();
}

bool SceneManager::CheckTransitions() {
    if (!current || !map || !player) return false;

    SDL_Rect bounds = player->GetBounds();
    for (const auto& transition : current->transitions) {
        if (!transition.automatic) continue;
        for (const auto& rect : map->TriggerRects(transition.trigger)) {
            if (Physics::CheckCollision(bounds, rect)) {
                RequestScene(transition.target, transition.spawn, transition.params);
                return true;
            }
        }
    }
    return false;
}

void SceneManager::FollowPlayer() {
    if (!map || !player) return;

    SDL_Rect bounds = player->GetBounds();
    int targetX = bounds.x - EngineConfig::SCREEN_WIDTH / 2;
    int targetY = bounds.y - EngineConfig::SCREEN_HEIGHT / 2;
    int maxX = map->WidthPx() - EngineConfig::SCREEN_WIDTH;
    int maxY = map->HeightPx() - EngineConfig::SCREEN_HEIGHT;

    Game::cameraX_float += (targetX - Game::cameraX_float) * 0.1f;
    Game::camera.x = static_cast<int>(Game::cameraX_float);
    Game::camera.y = targetY;

    if (Game::camera.x < 0) Game::camera.x = 0;
    if (Game::camera.y < 0) Game::camera.y = 0;
    if (maxX > 0 && Game::camera.x > maxX) Game::camera.x = maxX;
    if (maxY > 0 && Game::camera.y > maxY) Game::camera.y = maxY;
    if (maxX <= 0) { Game::camera.x = 0; Game::cameraX_float = 0.0f; }
    if (maxY <= 0) Game::camera.y = 0;
}

void SceneManager::HandleEvent(SDL_Event& event) {
    if (controller) controller->HandleEvent(context, event);
}

void SceneManager::Update() {
    if (!view) return;

    if (controller) controller->Update(context);
    if (!CheckTransitions()) FollowPlayer();

    if (hasPending) {
        std::string scene = pendingScene;
        std::string spawn = pendingSpawn;
        json params = pendingParams;
        hasPending = false;
        LoadScene(scene, spawn, params);
    }
}

void SceneManager::Render() {
    if (view) view->Render(context);
}
