开始 Stage 7C：Componentized + Data-driven Menu & Interaction System。

背景：
当前引擎已具备：

* Game Loop / Time
* Input Mapping
* SceneManager
* Entity + Behavior
* Physics / Collision
* TileMap
* Resource System
* Animation
* Audio
* OpenGL Renderer
* Shader / PostProcess
* Bézier Page Curl
* Red Border Effect

本阶段目标：

**实现一个轻量、组件化、数据驱动、可配置的 Menu / Interaction UI 系统。**

完成后，MyEngine 的核心 runtime 可视为基本定型。

==================================================
核心设计原则
======

1. Menu 是 UI / Interaction 表现层，不承载游戏规则。
2. 配置决定“显示什么”。
3. Controller / Behavior 决定“操作以后发生什么”。
4. Menu 既能用于：

   * Main Menu
   * Pause Menu
   * Settings
   * Confirm Dialog
   * NPC Choice
   * Inspect / Interaction Prompt
   * Shop / Selection
   * Chapter Select
5. 不做完整 GUI framework。
6. 不做 Godot Control / Unity UI 的复制品。
7. 必须对 Agent 友好：

   * JSON 可配置
   * 可 grep
   * 可 diff
   * 不依赖编辑器状态
8. 不顺手实现与 Menu 无关的系统。

==================================================
Stage 7C — Step 0：Audit
=======================

先审计，不直接编码。

检查：

* 当前 TextRenderer
* Renderer::DrawRect / DrawSprite
* Input Mapping
* Audio volume API
* Shader / SpriteEffect / PostProcess
* SceneView / SceneController
* Behavior
* SceneContext
* config JSON 解析方式
* 当前游戏内已有的：

  * 提示文字
  * 选择
  * HUD
  * 暂停/设置需求
  * 互动行为

重点回答：

1. 当前最小 UI primitive 是否已足够：

   * text
   * rect
   * image

2. 是否需要新的 UIElement 基类？

3. Menu ownership 应放在哪里？
   推荐优先：

   * MenuManager / UIManager 独立系统
     而不是塞进 SceneManager。

4. Menu 是否应该：

   * scene-owned
   * global stack
   * 或二者结合

5. Menu 如何与 Input Mapping 对接？

6. Menu 如何把 action 返回给 Controller / Behavior？

7. UI 是否默认在 PostFX 后绘制？

8. 如何允许部分 effect 覆盖 UI？

9. 当前 TextRenderer 是否支持：

   * text size
   * alignment
   * color
     如果不支持，最小需要补哪些能力？

10. interaction system 当前是否已有可复用入口？

完成 Audit 后先报告方案，不自动编码。

==================================================
Step 1：UI Core Components
=========================

实现最小 UI component model。

建议：

```cpp
class UIElement {
public:
    std::string id;

    bool visible = true;
    bool enabled = true;
    bool focusable = false;

    Rect rect;

    virtual void Update(float dt) {}
    virtual void Render() = 0;
};
```

允许根据当前代码风格调整。

第一版组件：

```text
Label
Image
Panel
Button
Toggle
Slider
Spacer
```

不要增加更多。

==================================================
UIElement 公共属性
==============

至少支持：

```text
id
visible
enabled

position
size

anchor
offset

opacity
tint
```

focusable 仅对可交互组件有效。

==================================================
Anchor
======

至少：

```text
top_left
top_center
top_right

center_left
center
center_right

bottom_left
bottom_center
bottom_right
```

screen resize 后位置正确。

不做复杂 constraint layout。

==================================================
Step 2：Layout
=============

只实现：

```text
VerticalLayout
HorizontalLayout
```

支持：

```text
spacing
padding
alignment
```

允许：

```json
{
  "layout": {
    "type": "vertical",
    "anchor": "center",
    "spacing": 16
  }
}
```

不要做：

* flexbox
* grid
* percentage layout
* constraint solver

如果后续游戏实际需要再加。

==================================================
Step 3：Menu
===========

Menu 是 UIElement 的容器。

建议结构：

```cpp
class Menu {
public:
    Open();
    Close();

    Update(float dt);
    Render();

    FocusNext();
    FocusPrev();

    Confirm();
    Cancel();

    UIElement* Focused();
};
```

Menu 必须维护：

```text
elements
focus order
current focus
visible/open state
```

==================================================
Focus 规则
========

必须：

* hidden item 跳过
* disabled item 跳过
* non-focusable item 跳过
* menu open 时自动选择第一个有效元素
* focus 被禁用/隐藏后自动迁移
* 没有 focusable item 时不 crash

==================================================
Step 4：Input Integration
========================

Menu 不允许直接使用 SDL_SCANCODE。

必须走现有 Input Mapping。

使用：

```text
UIUp
UIDown
UILeft
UIRight
UIConfirm
UICancel
```

默认映射可以是：

```text
W / Up
S / Down
A / Left
D / Right
Enter / Space
Escape
```

未来 gamepad 可复用。

==================================================
Input Context
=============

建议加入非常轻量的 input context / priority：

```text
Gameplay
Menu
```

当 Menu 打开：

```text
Menu input 优先
Gameplay input 可暂停或屏蔽
```

不要引入复杂 input routing framework。

至少防止：

按 Enter 确认菜单时，
同时触发 gameplay 的 Interact。

==================================================
Step 5：Interactive Components
=============================

Button：

```text
focused
pressed
action id
```

Toggle：

```text
bool value
left/right 或 confirm 切换
```

Slider：

```text
min
max
step
value
```

支持：

```text
UILeft / UIRight
```

Label/Image/Panel 不参与 focus。

==================================================
Step 6：Action Model
===================

Menu 不知道游戏逻辑。

定义简单 action：

```cpp
struct UIAction {
    std::string menuId;
    std::string elementId;
    std::string action;
    json value;
};
```

或等价结构。

例如：

```text
menuId = pause
elementId = resume
action = activate
```

Slider：

```text
action = value_changed
value = 0.7
```

Toggle：

```text
action = value_changed
value = true
```

Menu 把 action 交给上层。

==================================================
Action 接收
=========

允许：

```cpp
SceneController::HandleUIAction(...)
```

或注册 callback。

优先选择与现有 Controller / Behavior 生命周期兼容的最小方式。

不要使用全局 Event Bus。

==================================================
Step 7：MenuManager / MenuStack
==============================

实现轻量 Menu Stack。

至少：

```cpp
Push(menu)
Pop()
Replace(menu)
Top()
Clear()
```

支持：

```text
Game
↓
Pause

Pause
↓
Settings

Settings
↓
Audio
```

Cancel：

```text
Audio -> Settings -> Pause -> Game
```

==================================================
Menu Stack 生命周期
===============

要求：

* Push 后旧 menu 可保留但不响应 input。
* Pop 后恢复上一层 focus。
* Clear 安全。
* scene transition 时 scene-local menu 正确释放。
* global menu 可选择保留。

第一版可以只实现一个 manager + ownership 策略。

但必须把 ownership 写清楚。

==================================================
Step 8：Data-driven Menu
=======================

Menu 必须能从 JSON 构建。

示例：

```json
{
  "id": "pause",
  "layout": {
    "type": "vertical",
    "anchor": "center",
    "spacing": 14
  },
  "elements": [
    {
      "type": "label",
      "id": "title",
      "text": "Paused"
    },
    {
      "type": "button",
      "id": "resume",
      "text": "Resume",
      "action": "resume"
    },
    {
      "type": "button",
      "id": "settings",
      "text": "Settings",
      "action": "open_settings"
    },
    {
      "type": "button",
      "id": "quit",
      "text": "Quit",
      "action": "quit"
    }
  ]
}
```

Settings：

```json
{
  "id": "settings",
  "layout": {
    "type": "vertical",
    "anchor": "center",
    "spacing": 12
  },
  "elements": [
    {
      "type": "slider",
      "id": "music",
      "text": "Music",
      "min": 0,
      "max": 1,
      "step": 0.05,
      "binding": "audio.music"
    },
    {
      "type": "slider",
      "id": "sfx",
      "text": "SFX",
      "min": 0,
      "max": 1,
      "step": 0.05,
      "binding": "audio.sfx"
    },
    {
      "type": "button",
      "id": "back",
      "text": "Back",
      "action": "back"
    }
  ]
}
```

==================================================
Step 9：Simple Binding
=====================

只实现非常小的 binding。

目标：

```text
audio.music
audio.sfx
```

可以自动：

* 读取初始值
* slider 修改后更新 Audio

也允许 gameplay 手动处理 action。

不要实现通用 reactive binding framework。

如果 binding 开始膨胀，立即停止。

==================================================
Step 10：Style / Theme
=====================

Menu 样式必须数据驱动。

最小 style：

```text
font
fontSize
textColor

backgroundColor

focusedColor
disabledColor

padding
spacing
```

允许 theme：

```json
{
  "theme": "default_dark"
}
```

但不要做 CSS。

==================================================
Button 状态
=========

至少：

```text
normal
focused
pressed
disabled
```

状态表现可以通过：

```text
color
scale
shader effect
```

==================================================
Step 11：Shader Effect Hook
==========================

这里要利用现有 shader 系统。

UI element / Menu 可配置：

```json
{
  "focus_effect": "menu_focus_glow"
}
```

或：

```json
{
  "open_effect": "fade_in"
}
```

第一版至少支持：

* focus effect
* open effect
* close effect

但只做“hook”。

不要实现复杂 timeline。

==================================================
已有 Shader 的使用边界
===============

允许：

* grayscale
* tint
* pulse
* red border
* simple distortion
* fade

Page Curl 是否用于 Menu open/close：
可以预留接口，但不要求本阶段必须接入。

==================================================
Step 12：Interaction Presenter
=============================

这是本阶段的重点之一。

目标：

游戏内 interaction 可以选择“如何表现”。

例如 Entity/Behavior 配置：

```json
{
  "interaction": {
    "type": "menu",
    "menu": "gift_actions"
  }
}
```

gift_actions：

```json
{
  "id": "gift_actions",
  "layout": {
    "type": "vertical",
    "anchor": "bottom_center"
  },
  "elements": [
    {
      "type": "button",
      "id": "open",
      "text": "打开",
      "action": "open"
    },
    {
      "type": "button",
      "id": "inspect",
      "text": "查看",
      "action": "inspect"
    },
    {
      "type": "button",
      "id": "leave",
      "text": "离开",
      "action": "cancel"
    }
  ]
}
```

==================================================
InteractionPresenter
====================

建议提供：

```text
ShowMenu(menuId)
ShowChoice(...)
ShowConfirm(...)
Hide()
```

或者等价接口。

Controller / Behavior 只关心结果。

例如：

```cpp
if (action == "open") {
    OpenGift();
}
```

Menu 不知道 GiftBox。

==================================================
Step 13：Interaction 类型可替换
=========================

目标：

同一个 gameplay interaction：

```text
Interact with entity
```

可以通过配置选择不同 presenter：

```json
"interaction": {
  "type": "instant"
}
```

或：

```json
"interaction": {
  "type": "menu",
  "menu": "..."
}
```

或：

```json
"interaction": {
  "type": "confirm",
  "text": "确定打开？"
}
```

也就是说：

**互动逻辑不绑定互动表现。**

这是本系统的重要设计目标。

==================================================
Step 14：Render Pipeline
=======================

UI 默认顺序：

```text
World
↓
World PostFX
↓
UI
↓
Final PostFX
↓
Screen
```

要求：

* UI 默认不参与 world distortion。
* Red Border 作为 FinalFX 可以覆盖 UI。
* UI 自己的 shader effect 只影响对应 UI element。
* UI render 不修改 SceneManager world render。

==================================================
Step 15：Pause
=============

Menu system 至少要支持：

```text
pause gameplay update
但仍然：
- UI update
- UI input
- UI render
- optional shader animation
```

可以利用：

```text
TimeScale = 0
UnscaledDeltaTime
```

但不要让菜单系统硬改全局 TimeScale，除非当前架构最合理。

优先让调用方控制 pause policy。

==================================================
Step 16：正式验收 Demo
=================

必须至少做三个真实 Demo。

A. Main Menu

```text
Continue
New Game
Settings
Quit
```

B. Pause Menu

```text
Resume
Settings
Main Menu
```

C. Interaction Menu

例如 GiftBox：

```text
Open
Inspect
Leave
```

必须证明：
同一个 interaction 可通过配置从：

```text
直接触发
```

切换成：

```text
弹出 menu 选择
```

而无需改底层 Menu 系统。

==================================================
Step 17：Settings 验收
===================

至少：

```text
Music Volume
SFX Volume
```

Slider 必须真实修改 Audio。

如果已有 Fullscreen API，可加 Toggle。

没有就不要为了 Demo 新造 window system。

==================================================
Step 18：Navigation 验收
=====================

必须验证：

* Up

* Down

* Left

* Right

* Confirm

* Cancel

* disabled 跳过

* hidden 跳过

* focus wrap 是否支持要明确

* menu stack

* push/pop

* focus restore

==================================================
Step 19：Data-driven 验收
======================

要求：

仅修改 JSON：

```text
调整菜单顺序
新增一个 Button
修改文本
调整 spacing
修改 slider step
修改 action id
```

都不需要重新修改 engine C++。

这是硬验收项。

==================================================
Step 20：Agent-friendly 验收
=========================

最终必须能做到：

给 Agent 一句：

“在暂停菜单 Settings 下面增加一个 Credits 按钮，点击进入 credits 菜单。”

正常情况下只需要：

* 修改 menu JSON
* 增加对应 action / controller 处理

不需要修改 Menu engine core。

==================================================
明确禁止
====

本阶段不要做：

* UI Editor
* DOM
* CSS
* Flexbox
* Constraint layout
* MVVM
* Reactive Framework
* Global Event Bus
* Visual scripting
* Drag/drop framework
* Rich text editor
* Docking
* Window system
* ImGui 集成
* Widget plugin framework

==================================================
实现状态（当前）
==============

已完成（Steps 0–7，全部位于 engine/，不含任何游戏逻辑）：

| Step | 内容 | 主要文件 |
| --- | --- | --- |
| 1 | UIElement + Label/Image/Panel/Button/Toggle/Slider/Spacer | engine/include/Engine/UI/UIElement.h、UIPrimitives.h |
| 2 | UILayout / VerticalLayout / HorizontalLayout / UIContainer | engine/include/Engine/UI/UILayout.h、UIContainer.h |
| 3 | Menu + focus 导航（跳过 disabled/hidden/non-focusable，支持 wrap） | engine/include/Engine/UI/Menu.h |
| 4 | Input 上下文（Gameplay/Menu）+ MenuInput 路由 | engine/include/Engine/Input.h、UI/MenuInput.h |
| 5 | 交互组件（Toggle/Slider 左右调节，Slider 长按连发） | engine/src/UI/UIPrimitives.cpp |
| 6 | UIAction 模型（menuId/elementId/action/value） | engine/include/Engine/UI/UIAction.h |
| 7 | MenuStack（Push/Pop/Replace/Clear，独占所有权） | engine/include/Engine/UI/MenuStack.h |

尚未完成（Steps 8–20）：JSON 菜单加载（UIManager）、simple binding、theme/style、
shader hook、InteractionPresenter、渲染管线接入、pause 语义、验收 demo。

结论：**菜单核心框架已完成，但尚未接入游戏**。当前 `engine/src/Game.cpp` 的 UI 阶段仍是
占位（World → composite → (UI) → Final），没有 UIManager、没有从 `config.json` 构建菜单、
也没有 pause 接线。

==================================================
使用说明（已实现 API）
==================

引擎不包含游戏逻辑；菜单只产出 action，由上层消费。

1) 代码构建一个菜单

```cpp
#include "Engine/UI/Menu.h"
#include "Engine/UI/UIPrimitives.h"

Menu menu;
menu.id = "pause";
menu.anchor = Anchor::Center;
menu.rect = {0, 0, 340, 460};

auto layout = std::make_unique<VerticalLayout>();
layout->spacing = 12;
layout->padding = 16;
layout->mainAlign = LayoutMainAlign::Center;
layout->crossAlign = LayoutCrossAlign::Center;
menu.layout = std::move(layout);

auto title = std::make_unique<Label>();
title->id = "title"; title->text = "Paused"; title->fontSize = 32;
menu.Add(std::move(title));

auto resume = std::make_unique<Button>();
resume->id = "resume"; resume->text = "Resume"; resume->action = "resume";
menu.Add(std::move(resume));

auto music = std::make_unique<Slider>();
music->id = "music"; music->text = "Music";
music->minValue = 0.0f; music->maxValue = 1.0f; music->step = 0.05f;
music->SetValue(Audio::MusicVolume());
menu.Add(std::move(music));

menu.Open();   // 显示并自动 focus 第一个可 focus 元素
```

2) 输入路由

```cpp
// 每帧：只有栈顶菜单响应；使用 Input 的语义动作，不直接读 SDL_SCANCODE
Input::SetContext(Input::InputContext::Menu);   // 打开菜单时切到 Menu 上下文
MenuInput::Handle(menu, dt);                     // UIUp/UIDown/UILeft/UIRight/UIConfirm/UICancel
```

Input 上下文保证菜单打开时按 Enter 不会同时触发 gameplay 的 Interact（反之亦然）。

3) MenuStack

```cpp
MenuStack menus;
menus.Push(CreatePauseMenu());       // 接管所有权；下层保留但不响应输入
menus.Push(CreateSettingsMenu());    // Settings 成为 top

menus.HandleInput(dt);               // 只有 top 响应
menus.Update(dt);
menus.Render();                      // 从底到顶绘制

menus.Pop();                         // 回到 Pause 并恢复其 focus
menus.Replace(CreateAudioMenu());    // 替换 top（销毁旧 top）
menus.Clear();                       // 释放全部，安全
```

Ownership：MenuStack 独占所持 `Menu`（`unique_ptr`）；`Top()` 返回非拥有指针。

4) 消费 UIAction

```cpp
for (const UIAction& a : menus.Top()->ConsumeActions()) {
    // a.menuId / a.elementId / a.action / a.value
    // action: "activate" | "value_changed" | "confirm" | "cancel"
    controller->HandleUIAction(a);   // 或全局 handler
}
```

不要使用全局 Event Bus。

==================================================
实现顺序
====

建议严格按：

Step 0
Audit

Step 1
UIElement + primitive components

Step 2
Layout

Step 3
Menu + Focus

Step 4
Input integration

Step 5
Button/Toggle/Slider

Step 6
Action model

Step 7
MenuStack

Step 8
JSON menu loading

Step 9
Simple binding

Step 10
Theme/style

Step 11
Shader hook

Step 12
InteractionPresenter

Step 13
Interaction presentation replacement

Step 14
Render pipeline integration

Step 15
Pause semantics

Step 16+
真实 demo / verification

==================================================
每个 Step 规则
==========

每一步：

1. Audit
2. Plan
3. Implement
4. Build
5. Smoke
6. Demo/visual verification
7. Commit
8. Report

不要多个大步骤一起做。

==================================================
构建验证
====

每一步至少：

* main clean build
* feature/memory-island clean build
* feature/birthday-rpg clean build

关键步骤：

* memory-island smoke
* birthday-rpg smoke

UI 接入后必须人工验证。

==================================================
Commit 建议
=========

例如：

`feat: add ui core components`

`feat: add menu navigation and focus`

`feat: add data driven menu definitions`

`feat: add menu stack and actions`

`feat: add interaction menu presenter`

不要 squash 成一个大 commit。

==================================================
最终完成标准
======

Stage 7C 完成后，必须能证明：

1. UI 是组件化的。
2. Menu 是配置化的。
3. Menu 是数据驱动的。
4. Gameplay 逻辑不写进 Menu。
5. Menu 可用于主菜单/暂停/设置。
6. Menu 可替换 Entity interaction 表现。
7. Audio slider 真正可用。
8. Input Mapping 真正接入。
9. Shader effect 可以挂到 UI。
10. Agent 能通过改 JSON 快速改菜单。

==================================================
引擎冻结条件
======

Stage 7C 验收通过后：

MyEngine core 进入 feature freeze。

核心能力视为：

```text
Game Loop / Time
Input Mapping
SceneManager
Entity + Behavior
Physics
TileMap
Resource
Animation
Audio
Renderer
Shader / PostProcess
Page Curl
Red Border
Menu / UI / Interaction
```

此后：

不再以“让引擎更完整”为理由增加系统。

只根据真实游戏需求增加能力。

如果未来出现：

* Save
* Particle
* Camera Shake
* Tween
* 更多 Shader
* Dialogue
* Localization

全部按游戏需求独立新增。

现在先执行 Stage 7C — Step 0：Audit。

完成 Audit 后暂停，只提交：

* 当前 UI/互动现状
* ownership 建议
* MenuManager / MenuStack 方案
* JSON schema 建议
* Action 传播方案
* Render pipeline 接入点
* InteractionPresenter 方案
* 预计修改文件

不要直接编码。
