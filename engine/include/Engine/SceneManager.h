#pragma once
#include <SDL.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "Engine/Entity.h"
#include "Engine/SceneContext.h"
#include "Engine/SceneData.h"
#include "Engine/TileMap.h"
#include "Engine/TileSet.h"
#include "Engine/json.hpp"

class SceneView;
class SceneController;

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

    bool Active() const { return view != nullptr; }
    TileMap* Map() { return map.get(); }
    const SceneData* Current() const { return current; }

    std::vector<SDL_Rect> Colliders() const;
    bool CheckTransitions();
    void FollowPlayer();

private:
    void LoadScene(const std::string& sceneId, const std::string& spawn, const json& runtimeParams);
    void ClearScene();
    void FlushPending();
    void RemoveDead();

    std::map<std::string, TileSet> tilesets;
    std::map<std::string, SceneData> scenes;
    std::string initialScene;
    std::string initialSpawn = "default";

    const SceneData* current = nullptr;
    std::unique_ptr<TileMap> map;
    std::unique_ptr<SceneView> view;
    std::unique_ptr<SceneController> controller;
    SceneContext context;

    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<Entity>> pendingInsert;
    bool iterating = false;
    float deltaTime = 0.0f;
    Uint32 lastTick = 0;

    bool hasPending = false;
    std::string pendingScene;
    std::string pendingSpawn = "default";
    json pendingParams;
};
