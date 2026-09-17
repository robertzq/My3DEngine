#include "Engine/SceneManager.h"
#include <cstdio>
#include <algorithm>
#include "Engine/BehaviorRegistry.h"
#include "Engine/Config.h"
#include "Engine/Game.h"
#include "Engine/Input.h"
#include "Engine/Log.h"
#include "Engine/Physics.h"
#include "Engine/PostProcess.h"
#include "Engine/RenderTarget.h"
#include "Engine/ResourceManager.h"
#include "Engine/Renderer.h"
#include "Engine/SceneController.h"
#include "Engine/SceneRegistry.h"
#include "Engine/SceneView.h"
#include "Engine/ShaderManager.h"
#include "Engine/SpriteEffect.h"
#include "Engine/Time.h"
#include "Engine/GL.h"
#include <cmath>

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() { ClearScene(); }

bool SceneManager::LoadConfig(const std::string& configResourceId) {
    std::string content = ResourceManager::GetText(configResourceId);
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

    if (config.contains("shaders") && config["shaders"].is_object()) {
        for (auto it = config["shaders"].begin(); it != config["shaders"].end(); ++it) {
            std::string vert = it.value().value("vertex", "");
            std::string frag = it.value().value("fragment", "");
            if (!vert.empty() && !frag.empty()) {
                ShaderManager::Load(it.key(), vert, frag);
            } else {
                LOG_WARN("SceneManager: shader 配置缺少 vertex/fragment -> " << it.key());
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
            pendingEffect = transition.transitionEffect;
            pendingEffectParams = transition.transitionParams;
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

    // 数据驱动 sprite shader：按 id 从 ShaderManager 取，创建 entity 拥有的 effect 实例
    if (!def.shader.empty()) {
        Shader* shader = ShaderManager::Get(def.shader);
        if (shader) {
            entity->effectStorage = std::make_unique<SpriteEffect>(shader);
            if (def.shaderParams.is_object()) {
                for (auto p = def.shaderParams.begin(); p != def.shaderParams.end(); ++p) {
                    const json& v = p.value();
                    if (v.is_boolean()) entity->effectStorage->SetInt(p.key(), v.get<bool>() ? 1 : 0);
                    else if (v.is_number_integer()) entity->effectStorage->SetInt(p.key(), v.get<int>());
                    else if (v.is_number()) entity->effectStorage->SetFloat(p.key(), v.get<float>());
                    else if (v.is_array() && v.size() == 2) entity->effectStorage->SetVec2(p.key(), v[0].get<float>(), v[1].get<float>());
                    else if (v.is_array() && v.size() == 3) entity->effectStorage->SetVec3(p.key(), v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
                    else if (v.is_array() && v.size() == 4) entity->effectStorage->SetVec4(p.key(), v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>());
                }
            }
            entity->sprite.effect = entity->effectStorage.get();
        } else {
            LOG_WARN("SceneManager: 未知 shader id -> " << def.shader << " (fallback 默认 sprite shader)");
        }
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

    if (!data.tileset.empty()) {
        auto tileSetIt = tilesets.find(data.tileset);
        if (tileSetIt == tilesets.end()) {
            LOG_ERROR("SceneManager: 未知图块集 -> " << data.tileset);
        } else if (!data.mapLayers.empty()) {
            // Phase3：多层地图（map.layers[]）。校验失败则报错，不静默降级。
            map = std::make_unique<WorldMap>();
            if (!map->LoadLayered(data.mapLayers, tileSetIt->second)) {
                LOG_ERROR("SceneManager: 多层地图加载失败，场景 " << sceneId << " 无有效地图");
                map.reset();
            }
        } else if (!data.map.empty()) {
            // 兼容旧格式：map 为字符串 -> 默认 ground 层。
            map = std::make_unique<WorldMap>();
            map->LoadLegacyMap(data.map, tileSetIt->second);
        }
    }

    Game::camera = {0, 0, EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT};
    Game::cameraX_float = 0.0f;

    context.game = Game::instance();
    context.manager = this;
    context.data = &data;
    if (data.projectionMode == "iso") { projection.mode = ProjectionMode::Iso; }
    else { projection.mode = ProjectionMode::Ortho; }
    projection.view.scale = data.projectionScale;
    context.projection = projection;

    // Elevation: 如场景配置了可选高度矩阵，在地图加载成功后加载。
    // 默认步长根据等距几何推导：一级高度 = 菱形半高 * scale；
    // 这保证抬一级正好让菱形叠上下一层菱形（像堆叠），视觉自然无悬空。
    // 失败时仅记录错误，不影响场景进入（高度是可选增强）。
    if (map && !data.elevationFile.empty()) {
        float derivedStep = 0.0f;
        if (projection.mode == ProjectionMode::Iso) {
            derivedStep = static_cast<float>(projection.grid.halfH()) * projection.view.scale;
        } else {
            derivedStep = static_cast<float>(map->TileSize());
        }
        int step = data.elevationStepPx > 0
                   ? data.elevationStepPx
                   : static_cast<int>(derivedStep > 0.0f ? derivedStep + 0.5f : 20.0f);
        map->SetElevationStep(step);
        if (!map->LoadElevation(data.elevationFile)) {
            LOG_WARN("SceneManager: 高度加载失败，场景 " << sceneId
                     << " 将以 0 高度（平地）运行 -> " << data.elevationFile);
        }
    }

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
                pendingEffect = transition.transitionEffect;
                pendingEffectParams = transition.transitionParams;
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

    if (projection.mode == ProjectionMode::Iso) {
        // 等距相机：把玩家脚底中心投影成屏幕坐标，并将 view.offset 设为
        // 视口中心与该投影的差值，使玩家始终稳在屏幕中央。
        float px = player->transform.x + static_cast<float>(player->transform.w) / 2.0f;
        float py = player->transform.y + static_cast<float>(player->transform.h);
        SDL_Point iso = projection.IsoPoint(px, py);
        // Elevation 相机补偿：玩家站在高地时，脚底已被抬高 elev*step，
        // 相机 offset 必须同步上移，否则人物推而跑出屏幕中心。
        float cameraElev = 0.0f;
        if (map && map->HasElevation()) {
            const int tile = map->TileSize();
            const int prow = static_cast<int>(py / tile);
            const int pcol = static_cast<int>(px / tile);
            cameraElev = static_cast<float>(map->GetElevation(pcol, prow)) * map->ElevationStep();
        }
        projection.view.offsetX = EngineConfig::SCREEN_WIDTH / 2.0f - iso.x * projection.view.scale;
        projection.view.offsetY = EngineConfig::SCREEN_HEIGHT / 2.0f - (iso.y * projection.view.scale - cameraElev);
        return;
    }

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
        std::string effect = pendingEffect;
        json effectParams = pendingEffectParams.is_object() ? pendingEffectParams : json::object();
        hasPending = false;
        pendingEffect.clear();
        pendingEffectParams = json::object();

        if (effect == "page_curl") {
            int w = Renderer::Width();
            int h = Renderer::Height();
            if (w <= 0 || h <= 0) { w = EngineConfig::SCREEN_WIDTH; h = EngineConfig::SCREEN_HEIGHT; }
            if (!transitionOldRT) transitionOldRT = std::make_unique<RenderTarget>();
            transitionOldRT->Resize(w, h);
            CaptureSceneTo(*transitionOldRT);      // 快照旧场景（像素，不保留任何 gameplay 对象）

            LoadScene(scene, spawn, params);       // 立刻切到新场景（下方为实时新场景）

            transitionActive = true;
            transitionElapsed = 0.0f;
            transitionDuration = effectParams.value("duration", 0.9f);
            transitionParams = MeshPageCurlParams{};
            transitionParams.progress = 0.0f;
            transitionParams.curlRadius = effectParams.value("curlRadius", 0.14f);
            transitionParams.curvature = effectParams.value("curvature", 0.8f);
            transitionParams.shadowStrength = effectParams.value("shadowStrength", 0.7f);
            transitionParams.highlightStrength = effectParams.value("highlightStrength", 0.35f);
            transitionParams.backsideDarken = effectParams.value("backsideDarken", 0.18f);
            transitionParams.tessellation = effectParams.value("tessellation", 64);
            transitionParams.originX = effectParams.value("originX", 1.0f);
            transitionParams.originY = effectParams.value("originY", 1.0f);
            transitionParams.dragX = effectParams.value("dragX", transitionParams.originX);
            transitionParams.dragY = effectParams.value("dragY", transitionParams.originY);
        } else {
            LoadScene(scene, spawn, params);       // 未声明 transition：瞬时切换
        }
    }

    if (transitionActive) {
        transitionElapsed += Time::DeltaTime();
        if (transitionElapsed >= transitionDuration) transitionActive = false;
    }
}

void SceneManager::CaptureSceneTo(RenderTarget& target) {
    target.Bind();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    Render();                 // 把当前 view 的画面画进 RT
    target.Unbind();
    Renderer::CheckError("transition capture");
}

void SceneManager::RenderTransition(int width, int height) {
    if (!transitionActive || !transitionOldRT || !transitionOldRT->Valid()) return;
    if (!PostProcess::CompositeTexture()) return;

    float t = transitionDuration > 0.0f ? transitionElapsed / transitionDuration : 1.0f;
    t = std::min(std::max(t, 0.0f), 1.0f);
    // easeOutCubic：立刻起步、结尾自然收，避免开头“停顿感”。折痕几何仍由 Bézier 决定。
    float e = 1.0f - std::pow(1.0f - t, 3.0f);
    transitionParams.progress = e;

    const Texture* newScene = PostProcess::WorldTexture();
    if (!newScene) return;
    MeshPageCurl::Render(transitionOldRT->ColorTexture(), newScene, width, height, transitionParams);
}

void SceneManager::DrawWorld() {
    bool iso = (projection.mode == ProjectionMode::Iso);
    SDL_Rect drawCam = iso ? SDL_Rect{0, 0, EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT}
                                : Game::camera;
    if (map) {
        if (iso) map->DrawGroundIso(projection, drawCam);
        else map->DrawGround(drawCam);
    }

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
            if (iso) {
                // 统一投影：overlay 以所在格脚底中点为锚投影，贴图向上延伸，并叠加该 cell 的高度。
                const SDL_Rect& tr = item.tile->rect;
                float fwx = static_cast<float>(tr.x) + static_cast<float>(tr.w) * 0.5f;  // 脚底水平中点
                float fwy = static_cast<float>(tr.y) + static_cast<float>(tr.h);         // 脚底底边
                int eLevel = 0;
                if (map && map->HasElevation() && item.tile->col >= 0 && item.tile->row >= 0) {
                    eLevel = map->GetElevation(item.tile->col, item.tile->row);
                }
                SDL_Point op = projection.ProjectedFoot(fwx, fwy, eLevel, map ? map->ElevationStep() : 0);
                int dx = op.x - tr.w / 2 - drawCam.x;   // 以脚底中点为底，水平居中
                int dy = op.y - tr.h - drawCam.y;        // 贴图向上延伸脚底落地
                if (dx < -tr.w || dx > drawCam.w || dy < -tr.h || dy > drawCam.h) continue;
                SDL_Rect dest{dx, dy, tr.w, tr.h};
                Renderer::DrawSprite(item.tile->texture, SDL_Rect{0, 0, 0, 0}, dest, SDL_FLIP_NONE);
            } else {
                SDL_Rect dest = item.tile->rect;
                dest.x -= drawCam.x;
                dest.y -= drawCam.y;
                if (dest.x < -dest.w || dest.x > drawCam.w ||
                    dest.y < -dest.h || dest.y > drawCam.h) continue;
                Renderer::DrawSprite(item.tile->texture, SDL_Rect{0, 0, 0, 0}, dest, SDL_FLIP_NONE);
            }
        } else if (item.entity) {
            const Entity* e = item.entity;
            if (iso) {
                // 等距：以脚底中心为锚，按投影定位。
                // 取实体逻辑坐标（像素）为脚底；贴图向上延伸整个高度。
                float footX = e->transform.x + static_cast<float>(e->transform.w) / 2.0f;
                float footY = e->transform.y + static_cast<float>(e->transform.h);
                int entityElevLevel = 0;
                if (map && map->HasElevation()) {
                    const int t = map->TileSize();
                    entityElevLevel = map->GetElevation(
                        static_cast<int>(static_cast<int>(footX) / t),
                        static_cast<int>(static_cast<int>(footY) / t));
                }
                SDL_Point foot = projection.ProjectedFoot(footX, footY, entityElevLevel, map ? map->ElevationStep() : 0);
                int sx = foot.x - drawCam.x;
                int sy = foot.y - drawCam.y - e->transform.h;  // 贴图顶在脚尖上方（地形高度已由统一投影计入）

                // 脚下阴影：在地面上画一个半透明深色椭圆，让实体“落地”。
                DrawFootShadow(foot.x - drawCam.x, foot.y - drawCam.y,
                               e->transform.w, projection.view.scale);

                // 裁剪（用围盒）
                if (sx < -e->transform.w || sx > Game::camera.w ||
                    sy < -e->transform.h || sy > Game::camera.h) continue;

                SDL_Rect src = (e->sprite.src.w > 0 && e->sprite.src.h > 0)
                                   ? e->sprite.src : SDL_Rect{0, 0, 0, 0};
                SDL_Rect dest{sx, sy, e->transform.w, e->transform.h};
                Renderer::DrawSprite(e->sprite.texture, src, dest,
                                     (e->sprite.flip & SDL_FLIP_VERTICAL) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
            } else {
                item.entity->Render(Game::camera);
            }
        }
    }
}

// ---- 等距脚下阴影（椭圆，懒生成） ----
static Texture* sShadowTex = nullptr;

static Texture* EnsureShadowTexture() {
    if (sShadowTex) return sShadowTex;
    const int W = 64, H = 32;
    SDL_Surface* surf = SDL_CreateRGBSurface(0, W, H, 32,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if (!surf) return nullptr;
    if (SDL_MUSTLOCK(surf)) SDL_LockSurface(surf);
    // 中心黑、alpha 径向渐变到边缘 0（扁椭圆）
    Uint32* px = (Uint32*)surf->pixels;
    const float cxp = (W - 1) * 0.5f, cyp = (H - 1) * 0.5f;
    const float rx = W * 0.5f, ry = H * 0.5f;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float nx = (x - cxp) / rx, ny = (y - cyp) / ry;
            float d = nx * nx + ny * ny;          // 归一椭圆距离^2
            float a = (d < 1.0f) ? (1.0f - d) : 0.0f;  // 边缘渐隐
            a = a * a;                             // 更集中到中心
            Uint8 alpha = (Uint8)(a * 140.0f);     // 中心 ~140，边缘 ~0
            Uint32 c = (alpha << 24) | (20 << 16) | (16 << 8) | 12;  // 深蓝黑
            px[y * W + x] = c;
        }
    }
    if (SDL_MUSTLOCK(surf)) SDL_UnlockSurface(surf);
    sShadowTex = Renderer::CreateTextureFromSurface(surf);
    SDL_FreeSurface(surf);
    return sShadowTex;
}

void SceneManager::DrawFootShadow(int cx, int cy, int entW, float scale) {
    Texture* tex = EnsureShadowTexture();
    if (!tex) return;
    int w = std::max(8, (int)(entW * 0.9f));      // 阴影宽 ≈ 实体宽
    int h = std::max(4, w / 3);                   // 扁平
    Shader* sh = ShaderManager::Get("mesh_default");
    if (!sh) return;
    MeshVertex v[4];
    v[0] = { (float)(cx - w / 2), (float)(cy - h / 2), 0.0f, 1.0f, 0, 0 };  // 左上
    v[1] = { (float)(cx + w / 2), (float)(cy - h / 2), 1.0f, 1.0f, 0, 0 };  // 右上
    v[2] = { (float)(cx + w / 2), (float)(cy + h / 2), 1.0f, 0.0f, 0, 0 };  // 右下
    v[3] = { (float)(cx - w / 2), (float)(cy + h / 2), 0.0f, 0.0f, 0, 0 };  // 左下
    unsigned short idx[6] = {0, 1, 2, 0, 2, 3};
    Renderer::DrawMesh(*sh, v, 4, idx, 6, tex, MeshDrawOptions{ {255,255,255,255} });
}

void SceneManager::Render() {
    if (view) view->Render(context);
}
