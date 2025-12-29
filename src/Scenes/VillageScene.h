// src/Scenes/VillageScene.h
#pragma once
#include "Scene.h"
#include "RPGPlayer.h"
#include "Map.h"
#include "GiftBox.h"
#include <string>
#include <vector>

// 定义演出状态
enum class SceneState {
    PLAYING,        // 正常游戏
    SCANNING,       // 黑客扫描/人脸识别阶段
    SHOW_WORD_CLOUD // 心形词云展示阶段
};

// 词云标签结构体
struct CloudTag {
    std::string text;
    int targetX, targetY;   // 最终心形位置
    float currentX, currentY; // 当前动画位置
    int fontSize;
    SDL_Color color;
};

class VillageScene : public Scene {
public:
    // 增加一个可选参数：spawnAtEntrance，用于指示是否需要出生在特定点
    VillageScene(std::string mapFile, bool spawnAtEntrance = false);

    void OnEnter() override;
    void OnExit() override;
    void HandleEvents(SDL_Event& event) override;
    void Update() override;
    void Render() override;

private:
    std::string mapPath;
    bool shouldSpawnAtEntrance; // 是否需要寻找出生点

    RPGPlayer* player;
    Map* map;

// 【新增】用来管理场景里的礼物盒
    std::vector<GiftBox*> gifts;
    // 触发器列表
    std::vector<SDL_Rect> doorTriggers; // ID: 7 (进房子)
    std::vector<SDL_Rect> bossTriggers; // ID: 14 (打BOSS)
    std::vector<SDL_Rect> exitTriggers; // ID: 15 (出房子)

    // --- 演出相关变量 ---
    SceneState currentState = SceneState::PLAYING;
    Uint32 sequenceStartTime = 0; // 记录进入当前状态的时间

    // 扫描阶段
    std::vector<std::string> logLines; // 终端显示的日志
    int scanStage = 0; // 当前显示到第几行日志

    // 词云阶段
    std::vector<CloudTag> tags;
    bool isCloudFormed = false; // 词云是否已经聚拢完成

    // 辅助函数
    void TriggerScanSequence();     // 启动扫描
    void InitCloudSequence();       // 初始化词云数据
    void DrawTextHelper(std::string text, int x, int y, int size, SDL_Color color); // 简单的文字绘制封装
};