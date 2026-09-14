#pragma once
#include <functional>
#include <map>
#include <string>
#include "Engine/Behavior.h"

class BehaviorRegistry {
public:
    using Factory = std::function<Behavior*()>;

    static void Register(const std::string& name, Factory factory) {
        factories()[name] = std::move(factory);
    }

    static Behavior* Create(const std::string& name) {
        auto it = factories().find(name);
        return it == factories().end() ? nullptr : it->second();
    }

    struct Proxy {
        Proxy(const std::string& name, Factory factory) { Register(name, std::move(factory)); }
    };

private:
    static std::map<std::string, Factory>& factories() {
        static std::map<std::string, Factory> registry;
        return registry;
    }
};
