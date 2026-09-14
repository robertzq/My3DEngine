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

    bool Down(const std::string& action);
    bool Pressed(const std::string& action);
    bool Released(const std::string& action);
    bool AnyPressed();
    int Axis(const std::string& axis);
}
