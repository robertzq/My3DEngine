#pragma once
#include "Engine/json.hpp"
#include <string>

// 菜单/组件只产出 UIAction，自身不执行任何游戏逻辑。
// 上层（UIManager / SceneController::HandleUIAction）负责消费。
// 例：
//   { menuId="pause", elementId="resume", action="activate" }
//   { menuId="settings", elementId="music", action="value_changed", value=0.7 }
//   { menuId="settings", elementId="fullscreen", action="value_changed", value=true }
struct UIAction {
    std::string menuId;
    std::string elementId;
    std::string action;        // "activate" | "value_changed" | "confirm" | "cancel"
    nlohmann::json value;      // 可选负载
};
