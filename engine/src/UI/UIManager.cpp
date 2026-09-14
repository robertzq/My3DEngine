#include "Engine/UI/UIManager.h"
#include "Engine/UI/Menu.h"
#include "Engine/UI/MenuStack.h"
#include "Engine/UI/MenuInput.h"
#include "Engine/UI/UIPrimitives.h"
#include "Engine/UI/UILayout.h"
#include "Engine/Input.h"
#include "Engine/ResourceManager.h"
#include "Engine/Log.h"
#include <memory>

namespace {

nlohmann::json definitions = nlohmann::json::object();
MenuStack stack;
UIManager::UIActionHandler actionHandler;

Anchor ParseAnchor(const std::string& s) {
    if (s == "top_left") return Anchor::TopLeft;
    if (s == "top_center") return Anchor::TopCenter;
    if (s == "top_right") return Anchor::TopRight;
    if (s == "center_left") return Anchor::CenterLeft;
    if (s == "center_right") return Anchor::CenterRight;
    if (s == "bottom_left") return Anchor::BottomLeft;
    if (s == "bottom_center") return Anchor::BottomCenter;
    if (s == "bottom_right") return Anchor::BottomRight;
    return Anchor::Center;
}

SDL_Color ParseColor(const nlohmann::json& j, const SDL_Color& def) {
    if (!j.is_array() || j.size() < 3) return def;
    SDL_Color c = def;
    c.r = static_cast<Uint8>(j[0].get<int>());
    c.g = static_cast<Uint8>(j[1].get<int>());
    c.b = static_cast<Uint8>(j[2].get<int>());
    if (j.size() >= 4) c.a = static_cast<Uint8>(j[3].get<int>());
    return c;
}

LayoutMainAlign ParseMainAlign(const std::string& s) {
    if (s == "center") return LayoutMainAlign::Center;
    if (s == "end") return LayoutMainAlign::End;
    return LayoutMainAlign::Start;
}

LayoutCrossAlign ParseCrossAlign(const std::string& s) {
    if (s == "center") return LayoutCrossAlign::Center;
    if (s == "end") return LayoutCrossAlign::End;
    if (s == "stretch") return LayoutCrossAlign::Stretch;
    return LayoutCrossAlign::Start;
}

void ApplyCommon(UIElement* e, const nlohmann::json& j) {
    if (j.contains("id")) e->id = j["id"].get<std::string>();
    if (j.contains("visible")) e->visible = j["visible"].get<bool>();
    if (j.contains("enabled")) e->enabled = j["enabled"].get<bool>();
    if (j.contains("opacity")) e->opacity = j["opacity"].get<float>();
    if (j.contains("tint")) e->tint = ParseColor(j["tint"], e->tint);
    if (j.contains("offset") && j["offset"].is_array() && j["offset"].size() >= 2)
        e->offset = {j["offset"][0].get<int>(), j["offset"][1].get<int>()};
    if (j.contains("rect") && j["rect"].is_array() && j["rect"].size() >= 4)
        e->rect = {j["rect"][0].get<int>(), j["rect"][1].get<int>(),
                   j["rect"][2].get<int>(), j["rect"][3].get<int>()};
    if (j.contains("size") && j["size"].is_array() && j["size"].size() >= 2) {
        e->rect.w = j["size"][0].get<int>();
        e->rect.h = j["size"][1].get<int>();
    }
    if (j.contains("focusable")) e->focusable = j["focusable"].get<bool>();
}

std::unique_ptr<UILayout> ParseLayout(const nlohmann::json& l) {
    const std::string type = l.value("type", std::string("vertical"));
    std::unique_ptr<UILayout> lay;
    VerticalLayout* v = nullptr;
    HorizontalLayout* h = nullptr;
    if (type == "horizontal") {
        auto p = std::make_unique<HorizontalLayout>();
        h = p.get();
        lay = std::move(p);
    } else {
        auto p = std::make_unique<VerticalLayout>();
        v = p.get();
        lay = std::move(p);
    }

    const int spacing = l.value("spacing", -1);
    const int padding = l.value("padding", -1);
    const LayoutMainAlign ma = ParseMainAlign(l.value("mainAlign", std::string("start")));
    const LayoutCrossAlign ca = ParseCrossAlign(l.value("crossAlign", std::string("start")));
    if (v) {
        if (spacing >= 0) v->spacing = spacing;
        if (padding >= 0) v->padding = padding;
        v->mainAlign = ma;
        v->crossAlign = ca;
    }
    if (h) {
        if (spacing >= 0) h->spacing = spacing;
        if (padding >= 0) h->padding = padding;
        h->mainAlign = ma;
        h->crossAlign = ca;
    }
    return lay;
}

std::unique_ptr<UIElement> ParseElement(const nlohmann::json& j) {
    const std::string type = j.value("type", std::string());
    std::unique_ptr<UIElement> e;

    if (type == "label") {
        auto p = std::make_unique<Label>();
        p->text = j.value("text", std::string());
        p->fontSize = j.value("fontSize", 0);
        if (j.contains("color")) p->color = ParseColor(j["color"], p->color);
        e = std::move(p);
    } else if (type == "image") {
        auto p = std::make_unique<Image>();
        if (j.contains("texture")) p->texture = ResourceManager::GetTexture(j["texture"].get<std::string>());
        p->flipX = j.value("flipX", false);
        p->flipY = j.value("flipY", false);
        e = std::move(p);
    } else if (type == "panel") {
        auto p = std::make_unique<Panel>();
        if (j.contains("color")) p->color = ParseColor(j["color"], p->color);
        p->border = j.value("border", false);
        e = std::move(p);
    } else if (type == "button") {
        auto p = std::make_unique<Button>();
        p->text = j.value("text", std::string());
        p->action = j.value("action", std::string());
        p->fontSize = j.value("fontSize", 0);
        e = std::move(p);
    } else if (type == "toggle") {
        auto p = std::make_unique<Toggle>();
        p->text = j.value("text", std::string());
        p->value = j.value("value", false);
        p->fontSize = j.value("fontSize", 0);
        e = std::move(p);
    } else if (type == "slider") {
        auto p = std::make_unique<Slider>();
        p->text = j.value("text", std::string());
        p->minValue = j.value("min", 0.0f);
        p->maxValue = j.value("max", 1.0f);
        p->step = j.value("step", 0.05f);
        p->SetValue(j.value("value", p->minValue));
        p->fontSize = j.value("fontSize", 0);
        if (p->rect.w == 0) p->rect.w = 220;
        if (p->rect.h == 0) p->rect.h = 40;
        e = std::move(p);
    } else if (type == "spacer") {
        e = std::make_unique<Spacer>();
    } else {
        LOG_WARN("UIManager: 未知元素类型 -> " << type);
        return nullptr;
    }

    ApplyCommon(e.get(), j);
    return e;
}

std::unique_ptr<Menu> BuildMenu(const std::string& menuId) {
    if (!definitions.is_object() || !definitions.contains(menuId)) {
        LOG_WARN("UIManager: 未定义菜单 -> " << menuId);
        return nullptr;
    }
    const nlohmann::json& d = definitions[menuId];

    auto menu = std::make_unique<Menu>();
    menu->id = d.value("id", menuId);
    menu->anchor = ParseAnchor(d.value("anchor", std::string("center")));
    if (d.contains("rect") && d["rect"].is_array() && d["rect"].size() >= 4)
        menu->rect = {d["rect"][0].get<int>(), d["rect"][1].get<int>(),
                      d["rect"][2].get<int>(), d["rect"][3].get<int>()};
    else
        menu->rect = {0, 0, 340, 460};

    const std::string pause = d.value("pause", std::string("gameplay"));
    if (pause == "none") menu->pauseMode = UIPauseMode::None;
    else if (pause == "full") menu->pauseMode = UIPauseMode::Full;
    else menu->pauseMode = UIPauseMode::Gameplay;

    if (d.contains("layout")) menu->layout = ParseLayout(d["layout"]);

    if (d.contains("elements") && d["elements"].is_array()) {
        for (const auto& ej : d["elements"]) {
            auto e = ParseElement(ej);
            if (e) menu->Add(std::move(e));
        }
    }
    return menu;
}

}

namespace UIManager {

void Init() {
    definitions = nlohmann::json::object();
    stack.Clear();
}

void Clean() {
    stack.Clear();
    definitions = nlohmann::json::object();
}

void LoadMenus(const nlohmann::json& menus) {
    if (!menus.is_object()) {
        LOG_WARN("UIManager: menus 段不是对象");
        return;
    }
    for (auto it = menus.begin(); it != menus.end(); ++it) definitions[it.key()] = it.value();
}

bool HasMenu(const std::string& menuId) { return definitions.contains(menuId); }

void Push(const std::string& menuId) {
    auto menu = BuildMenu(menuId);
    if (menu) stack.Push(std::move(menu));
}

void Replace(const std::string& menuId) {
    auto menu = BuildMenu(menuId);
    if (menu) stack.Replace(std::move(menu));
}

bool Pop() { return stack.Pop(); }
void Clear() { stack.Clear(); }
Menu* Top() { return stack.Top(); }
bool Empty() { return stack.Empty(); }
std::size_t Size() { return stack.Size(); }

void HandleInput(float dt) {
    Input::SetContext(stack.Empty() ? Input::InputContext::Gameplay : Input::InputContext::Menu);
    stack.HandleInput(dt);
}

void Update(float dt) { stack.Update(dt); }
void Render() { stack.Render(); }

std::vector<UIAction> ConsumeActions() {
    if (Menu* m = stack.Top()) return m->ConsumeActions();
    return {};
}

const std::vector<UIAction>& PendingActions() {
    static const std::vector<UIAction> empty;
    if (Menu* m = stack.Top()) return m->PendingActions();
    return empty;
}

bool Active() { return !stack.Empty(); }

void SetActionHandler(UIActionHandler handler) { actionHandler = std::move(handler); }
void ClearActionHandler() { actionHandler = nullptr; }

void DispatchActions() {
    if (!actionHandler) return;   // 未注册则保留队列，供上层轮询 ConsumeActions
    std::vector<UIAction> acts = ConsumeActions();
    for (const UIAction& a : acts) actionHandler(a);
}

bool ShouldPauseGameplay() { return stack.ShouldPauseGameplay(); }
bool ShouldPauseAll() { return stack.ShouldPauseAll(); }

}
