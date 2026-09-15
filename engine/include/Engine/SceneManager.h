#pragma once
#include <SDL.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "Engine/Entity.h"
#include "Engine/Projection.h"
#include "Engine/MeshPageCurl.h"
#include "Engine/SceneContext.h"
#include "Engine/SceneData.h"
#include "Engine/TileMap.h"
#include "Engine/TileSet.h"
#include "Engine/json.hpp"

class SceneView;
class SceneController;
class RenderTarget;

// 场景与实体生命周期的 ownership 契约：
//
// * SceneManager 拥有当前场景的 map / view / controller（unique_ptr，随场景重建）。
// * SceneManager 拥有 entities / pendingInsert（vector<unique_ptr<Entity>>）。
// * Entity 拥有其 Behavior（unique_ptr）；Behavior::self 是非 owning 裸指针，指回所属 Entity。
// * SceneContext 内的 game/manager/controller/map/data 均为非 owning，
//   只在当前场景生命周期内有效。
// * Spawn / FindById / FindByTag / Player 返回的 Entity* 不得跨场景切换或实体销毁后继续持有。
// * Map() / Current() 的返回值不得跨 ClearScene / scene transition 持有。
// * Sprite.texture 由 ResourceManager 管理，Entity 不拥有纹理。
// * View / Controller / Behavior 不应长期缓存 Entity* 或 SceneContext&，
//   除非能明确保证其生命周期（当前约定：与所在场景同生命周期）。
class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    bool LoadConfig(const std::string& configResourceId);
    void Start();
    void Start(const std::string& sceneId, const std::string& spawn = "default");

    void RequestScene(const std::string& sceneId, const std::string& spawn = "default",
                      const json& runtimeParams = json::object());
    void RequestTransition(const std::string& trigger, const json& runtimeParams = json::object());

    SDL_Point SpawnPosition(const std::string& spawn, const json& runtimeParams = json::object()) const;

    Entity* Spawn(const std::string& behavior, const std::string& tag, float x, float y,
                  const json& params = json::object(), const std::string& id = "");
    Entity* SpawnDef(const EntityDef& def);
    void Destroy(Entity* entity);
    std::vector<Entity*> Entities();
    Entity* FindById(const std::string& id);
    Entity* FindByTag(const std::string& tag);
    Entity* Player() { return FindByTag("player"); }

    void HandleEvent(SDL_Event& event);
    void Update();
    void Render();
    void DrawWorld();
    void DrawFootShadow(int cx, int cy, int entW, float scale);   // 等距脚下椭圆阴影

    TileMap* Map() { return map.get(); }
    const SceneData* Current() const { return current; }

    std::vector<SDL_Rect> Colliders() const;
    bool CheckTransitions();
    void FollowPlayer();

    // 转场（Page Curl）：old scene 快照 -> 加载 new scene -> 翻页盖在新场景上
    bool InTransition() const { return transitionActive; }
    void RenderTransition(int width, int height);

private:
    void LoadScene(const std::string& sceneId, const std::string& spawn, const json& runtimeParams);
    void ClearScene();
    void FlushPending();
    void RemoveDead();
    void CaptureSceneTo(RenderTarget& target);

    std::map<std::string, TileSet> tilesets;
    std::map<std::string, SceneData> scenes;
    std::string initialScene;
    std::string initialSpawn = "default";

    const SceneData* current = nullptr;
    // 当前投影模式（等距时用于 TileMap/Entity 渲染定位）
    Projection projection;
    std::unique_ptr<TileMap> map;
    std::unique_ptr<SceneView> view;
    std::unique_ptr<SceneController> controller;
    SceneContext context;

    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<Entity>> pendingInsert;
    bool iterating = false;

    bool hasPending = false;
    std::string pendingScene;
    std::string pendingSpawn = "default";
    json pendingParams;
    std::string pendingEffect;
    json pendingEffectParams;

    // Page Curl 转场状态（old scene 快照纹理 + 进度）
    std::unique_ptr<RenderTarget> transitionOldRT;
    bool transitionActive = false;
    float transitionElapsed = 0.0f;
    float transitionDuration = 0.9f;
    MeshPageCurlParams transitionParams;
};
