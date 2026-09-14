#pragma once
#include <functional>
#include <map>
#include <string>
#include "Engine/SceneController.h"
#include "Engine/SceneView.h"

class SceneRegistry {
public:
    using ViewFactory = std::function<SceneView*()>;
    using ControllerFactory = std::function<SceneController*()>;

    static void RegisterView(const std::string& name, ViewFactory factory) {
        views()[name] = std::move(factory);
    }

    static void RegisterController(const std::string& name, ControllerFactory factory) {
        controllers()[name] = std::move(factory);
    }

    static SceneView* CreateView(const std::string& name) {
        auto it = views().find(name);
        return it == views().end() ? nullptr : it->second();
    }

    static SceneController* CreateController(const std::string& name) {
        auto it = controllers().find(name);
        return it == controllers().end() ? nullptr : it->second();
    }

    struct ViewProxy {
        ViewProxy(const std::string& name, ViewFactory factory) { RegisterView(name, std::move(factory)); }
    };

    struct ControllerProxy {
        ControllerProxy(const std::string& name, ControllerFactory factory) { RegisterController(name, std::move(factory)); }
    };

private:
    static std::map<std::string, ViewFactory>& views() {
        static std::map<std::string, ViewFactory> registry;
        return registry;
    }

    static std::map<std::string, ControllerFactory>& controllers() {
        static std::map<std::string, ControllerFactory> registry;
        return registry;
    }
};
