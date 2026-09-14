#pragma once

class Menu;

// 把 Input 映射层的 UI 动作路由到 Menu 的 focus/navigation。
// 保持 Menu 与输入解耦：Menu 只暴露 Focus/Confirm/Cancel，本 driver 负责读输入。
// 注意：全部走 Input 的语义动作（UIUp/UIDown/UIConfirm/UICancel），不直接读 SDL_SCANCODE。
namespace MenuInput {
    void Handle(Menu& menu);
}
