#include "Engine/SceneManager.h"
#include <algorithm>
#include "Engine/BehaviorRegistry.h"
#include "Engine/Config.h"
#include "Engine/Game.h"
#include "Engine/Input.h"
#include "Engine/Log.h"
#include "Engine/Physics.h"
#include "Engine/ResourceManager.h"
#include "Engine/SceneController.h"
#include "Engine/SceneRegistry.h"
#include "Engine/SceneView.h"
#include "Engine/Time.h"

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
                    def.overlay = tile.value().value("overlay", false);
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

    if (config.contains("input") && config["input"].is_object()) {
        const json& in = config["input"];
        if (in.contains("actions") && in["actions"].is_object()) {
            for (auto it = in["actions"].begin(); it != in["actions"].end(); ++it) {
                std::vector<std::string> keys;
                if (it.value().is_array()) {
                    for (const auto& k : it.value()) keys.push_back(k.get<std::string>());
                } else if (it.value().is_string()) {
                    keys.push_back(it.value().get<std::string>());
                }
                Input::Bind(it.key(), keys);
            }
        } else {
            // 扁平写法：{ "MoveLeft": ["A", "LEFT"], ... }
            for (auto it = in.begin(); it != in.end(); ++it) {
                if (it.key() == "axes" || !it.value().is_array()) continue;
                std::vector<std::string> keys;
                for (const auto& k : it.value()) keys.push_back(k.get<std::string>());
                Input::Bind(it.key(), keys);
            }
        }
        if (in.contains("axes") && in["axes"].is_object()) {
            for (auto it = in["axes"].begin(); it != in["axes"].end(); ++it) {
                if (it.value().is_array() && it.value().size() >= 2) {
                    Input::BindAxis(it.key(), it.value()[0].get<std::string>(),
                                    it.value()[1].get<std::string>());
                }
            }
        }
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

Entity* SceneManager::Spawn(const std::string& behavior, const std::string& tag, float x, float y,
                            const json& params, const std::string& id) {
    EntityDef def;
    def.id = id;
    def.behavior = behavior;
    def.tag = tag;
    def.x = static_cast<int>(x);
    def.y = static_cast<int>(y);
    def.params = params;
    return SpawnDef(def);
}

Entity* SceneManager::SpawnDef(const EntityDef& def) {
    auto entity = std::make_unique<Entity>();
    entity->id = def.id;
    entity->behaviorName = def.behavior;
    entity->tag = def.tag;
    entity->transform.x = static_cast<float>(def.x);
    entity->transform.y = static_cast<float>(def.y);
    entity->transform.w = def.w;
    entity->transform.h = def.h;
    entity->visible = def.visible;

    if (!def.texture.empty()) entity->sprite.texture = ResourceManager::GetTexture(def.texture);
    entity->sprite.src = def.src;
    if (def.hasCollider) {
        entity->collider.offset = def.collider;
        entity->collider.enabled = true;
    }

    if (!def.behavior.empty()) {
        entity->behavior.reset(BehaviorRegistry::Create(def.behavior));
        if (entity->behavior) {
            entity->behavior->self = entity.get();
            entity->behavior->config = def.params;
            entity->behavior->OnSpawn(context);
        } else {
            LOG_WARN("SceneManager: 未注册的行为 -> " << def.behavior);
        }
    }

    Entity* raw = entity.get();
    if (iterating) pendingInsert.push_back(std::move(entity));
    else entities.push_back(std::move(entity));
    return raw;
}

void SceneManager::Destroy(Entity* entity) {
    if (entity) entity->alive = false;
}

std::vector<Entity*> SceneManager::Entities() {
    std::vector<Entity*> result;
    result.reserve(entities.size());
    for (auto& entity : entities) {
        if (entity->alive) result.push_back(entity.get());
    }
    return result;
}

Entity* SceneManager::FindById(const std::string& id) {
    for (auto& entity : entities) {
        if (entity->alive && entity->id == id) return entity.get();
    }
    return nullptr;
}

Entity* SceneManager::FindByTag(const std::string& tag) {
    for (auto& entity : entities) {
        if (entity->alive && entity->tag == tag) return entity.get();
    }
    return nullptr;
}

void SceneManager::ClearScene() {
    if (view) view->OnExit();
    if (controller) controller->OnExit();
    view.reset();
    controller.reset();
    map.reset();
    entities.clear();
    pendingInsert.clear();
    current = nullptr;
}

void SceneManager::FlushPending() {
    if (pendingInsert.empty()) return;
    for (auto& entity : pendingInsert) entities.push_back(std::move(entity));
    pendingInsert.clear();
}

void SceneManager::RemoveDead() {
    for (auto it = entities.begin(); it != entities.end();) {
        if (!it->get()->alive) it = entities.erase(it);
        else ++it;
    }
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

    Game::camera = {0, 0, EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT};
    Game::cameraX_float = 0.0f;

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

    if (data.hasPlayer) {
        EntityDef playerDef = data.player;
        SDL_Point point = SpawnPosition(spawn, runtimeParams);
        playerDef.x = point.x;
        playerDef.y = point.y;
        if (playerDef.tag.empty()) playerDef.tag = "player";
        SpawnDef(playerDef);
    }

    for (const auto& entityDef : data.entities) {
        SpawnDef(entityDef);
    }

    if (controller) controller->OnEnter(context);
    if (view) view->OnEnter(context);

    Time::Reset();
    LOG_INFO("SceneManager: 进入场景 " << sceneId << " (spawn=" << spawn << ")");
}

std::vector<SDL_Rect> SceneManager::Colliders() const {
    return map ? map->Colliders() : std::vector<SDL_Rect>();
}

bool SceneManager::CheckTransitions() {
    if (!current || !map) return false;
    Entity* player = Player();
    if (!player) return false;

    SDL_Rect bounds = player->Bounds();
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
    Entity* player = Player();
    if (!map || !player) return;

    SDL_Rect bounds = player->Bounds();
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
    iterating = true;
    for (auto& entity : entities) {
        if (entity->alive && entity->behavior) entity->behavior->HandleEvent(context, event);
    }
    iterating = false;
    if (controller) controller->HandleEvent(context, event);
    FlushPending();
}

void SceneManager::Update() {
    if (!view) return;

    const float deltaTime = Time::DeltaTime();

    iterating = true;
    for (auto& entity : entities) {
        if (entity->alive && entity->behavior) entity->behavior->Update(context, deltaTime);
    }
    iterating = false;

    if (controller) controller->Update(context);

    FlushPending();
    RemoveDead();

    if (!CheckTransitions()) FollowPlayer();

    if (hasPending) {
        std::string scene = pendingScene;
        std::string spawn = pendingSpawn;
        json params = pendingParams;
        hasPending = false;
        LoadScene(scene, spawn, params);
    }
}

void SceneManager::DrawWorld() {
    if (map) map->DrawGround(Game::renderer, Game::camera);

    struct Item {
        float key;
        const TileSprite* tile;
        Entity* entity;
    };

    std::vector<Item> items;
    if (map) {
        for (const auto& tile : map->Overlays()) {
            items.push_back({static_cast<float>(tile.sortY), &tile, nullptr});
        }
    }
    for (auto& entity : entities) {
        if (entity->alive && entity->visible) {
            items.push_back({entity->SortKey(), nullptr, entity.get()});
        }
    }

    std::stable_sort(items.begin(), items.end(),
                     [](const Item& a, const Item& b) { return a.key < b.key; });

    for (const auto& item : items) {
        if (item.tile) {
            SDL_Rect dest = item.tile->rect;
            dest.x -= Game::camera.x;
            dest.y -= Game::camera.y;
            if (dest.x < -dest.w || dest.x > Game::camera.w ||
                dest.y < -dest.h || dest.y > Game::camera.h) continue;
            SDL_RenderCopy(Game::renderer, item.tile->texture, nullptr, &dest);
        } else if (item.entity) {
            item.entity->Render(Game::renderer, Game::camera);
        }
    }
}

void SceneManager::Render() {
    if (view) view->Render(context);
}
