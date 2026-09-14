#pragma once
#include "Engine/json.hpp"
#include "Engine/UI/UIAction.h"
#include <cstddef>
#include <string>
#include <vector>

class Menu;

// UI 子系统（引擎级、静态、独立于 SceneManager）。
// 职责：持有菜单 JSON 定义 + MenuStack；从定义构建 Menu；路由输入；产出 UIAction。
// 不含任何游戏逻辑：action 由上层（SceneController / game handler）消费。
//
// 生命周期：
//   * LoadMenus 只存定义，可多次调用合并/覆盖。
//   * Push/Pop/Replace/Clear 操作运行时菜单实例（MenuStack 独占所有权）。
//   * scene-local 菜单随场景 Clear 释放；global 菜单由上层决定是否保留。
namespace UIManager {
    void Init();                                  // 清空定义与栈
    void Clean();

    // menus 段：{ "menuId": { "id","anchor","layout","elements",... }, ... }
    void LoadMenus(const nlohmann::json& menus);
    bool HasMenu(const std::string& menuId);

    void Push(const std::string& menuId);         // 从定义构建实例并入栈
    void Replace(const std::string& menuId);
    bool Pop();
    void Clear();
    Menu* Top();
    bool Empty();
    std::size_t Size();

    // 每帧（建议 Game::update/render 调用）
    void HandleInput(float dt);                   // 仅 top；内部切换 Input 上下文
    void Update(float dt);
    void Render();

    // 本帧 top 产生的 action；ConsumeActions 取走后清空
    std::vector<UIAction> ConsumeActions();
    const std::vector<UIAction>& PendingActions();

    bool Active();                                // == !Empty()（不要用它当 pause 语义）
}
