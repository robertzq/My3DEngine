#pragma once
#include "Scene.h"
#include "ResourceManager.h"
#include <string>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

struct Gift {
    std::string name;
    int battery;
};

enum ShakeTarget {
    SHAKE_NONE = 0,
    SHAKE_PLAYER,
    SHAKE_ENEMY
};

class BattleScene : public Scene {
public:
    BattleScene();
    ~BattleScene();

    void OnEnter() override;
    void OnExit() override;
    void Update() override;
    void Render() override;
    void HandleEvents(SDL_Event& event) override;

private:
    void DrawHPBar(int x, int y, int current, int max, SDL_Color color);
    std::string GetDamageText(int damage);
    void LoadGifts();

    SDL_Texture* bgTexture;
    SDL_Texture* enemyTexture;
    SDL_Texture* playerTexture;
    SDL_Texture* uiBoxTexture;
    SDL_Texture* cursorTexture;

    enum BattleState {
        PLAYER_TURN,
        PLAYER_ANIM,
        ENEMY_TURN,
        ENEMY_ANIM,
        VICTORY,
        DEFEAT
    };
    BattleState currentState;

    int playerHP, maxPlayerHP;
    int enemyHP, maxEnemyHP;

    int menuIndex;
    int damageDealt;

    std::string messageLog;
    std::string effectLog;

    ShakeTarget shakeTarget = SHAKE_NONE;
    int shakeTimer = 0;

    int hugeGiftCD = 0;
    const int MAX_HUGE_GIFT_CD = 3;

    // === 礼物数据 ===
    std::vector<Gift> blindBoxPool; // 盲盒池
    std::vector<Gift> enemyGiftPool;// 敌人池 (包含所有普通礼物)

    Gift basicGift;     // 选项0: 单推
    Gift hugeGift;      // 选项1: 舰长一号
    Gift blindBoxGift;  // 选项2: 心动盲盒
    Gift starWishGift;  // 选项3: 星愿水晶球 (新增)
};