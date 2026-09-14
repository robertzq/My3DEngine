#pragma once
#include <SDL.h>
#include <string>
#include <vector>
#include "Engine/SceneController.h"

class Texture;

struct Gift {
    std::string name;
    int battery = 0;
};

enum ShakeTarget {
    SHAKE_NONE = 0,
    SHAKE_PLAYER,
    SHAKE_ENEMY
};

class BattleController : public SceneController {
public:
    void OnEnter(SceneContext& context) override;
    void OnExit() override;
    void Update(SceneContext& context) override;

    Texture* bgTexture = nullptr;
    Texture* enemyTexture = nullptr;
    Texture* playerTexture = nullptr;
    Texture* uiBoxTexture = nullptr;
    Texture* cursorTexture = nullptr;

    enum BattleState {
        PLAYER_TURN,
        PLAYER_ANIM,
        ENEMY_TURN,
        ENEMY_ANIM,
        VICTORY,
        DEFEAT
    };
    BattleState currentState = PLAYER_TURN;

    int playerHP = 8000, maxPlayerHP = 8000;
    int enemyHP = 8000, maxEnemyHP = 8000;
    int menuIndex = 0;

    std::string messageLog;
    std::string effectLog;

    ShakeTarget shakeTarget = SHAKE_NONE;
    int shakeTimer = 0;

    int hugeGiftCD = 0;
    const int MAX_HUGE_GIFT_CD = 3;

    std::vector<Gift> blindBoxPool;
    std::vector<Gift> enemyGiftPool;

    Gift basicGift;
    Gift hugeGift;
    Gift blindBoxGift;
    Gift starWishGift;

    std::string returnScene = "village";
    int returnX = -1;
    int returnY = -1;

private:
    void HandleInput(SceneContext& context);
    void LoadGifts();
    std::string DamageText(int damage) const;
    void LeaveBattle(SceneContext& context);

    int animTimer = 0;
};
