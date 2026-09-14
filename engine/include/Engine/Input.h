#pragma once
#include <SDL.h>
#include <string>
#include <vector>

// 输入抽象层：
// * 底层保留原始键盘查询（IsKeyDown），gameplay 不应直接使用 SDL_SCANCODE。
// * 上层提供 action 映射：把若干物理键绑定到一个语义动作，
//   支持 Down（按住）/ Pressed（本帧刚按下）/ Released（本帧刚松开）/ Axis（-1/0/1）。
//
// 每帧必须调用一次 Input::Update()（由 Game::update() 负责）来刷新快照。
namespace Input {
    // 原始查询：某个物理键当前是否被按住
    bool IsKeyDown(SDL_Scancode key);

    // 绑定：action -> 一组按键名（如 "A" / "LEFT" / "SPACE" / "RETURN"）
    void Bind(const std::string& action, const std::vector<std::string>& keys);
    // 绑定轴：axis -> (负向 action, 正向 action)，查询返回 -1 / 0 / 1
    void BindAxis(const std::string& axis, const std::string& negative, const std::string& positive);

    // 每帧刷新输入快照（由 Game::update() 调用一次）
    void Update();

    // 输入上下文 / 优先级：
    // * 名字以 "UI" 开头的 action 视为 UI 动作（UIUp/UIDown/UIConfirm...），其余视为 gameplay 动作。
    // * Gameplay 上下文只响应 gameplay 动作，Menu 上下文只响应 UI 动作。
    //   这样菜单打开时按确认键不会同时触发 gameplay 的 Interact/Confirm。
    // * 默认 Gameplay；UIManager 会在菜单栈非空时切到 Menu（见后续步骤）。
    enum class InputContext { Gameplay, Menu };
    void SetContext(InputContext context);
    InputContext GetContext();
    bool IsUIAction(const std::string& action);
    // 当前上下文下该 action 是否被接受（Down/Pressed/Released/Axis 内部都走这个判定）
    bool ActionAllowed(const std::string& action);

    bool Down(const std::string& action);
    bool Pressed(const std::string& action);
    bool Released(const std::string& action);
    bool AnyPressed();
    int Axis(const std::string& axis);
}
