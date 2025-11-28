#pragma once
#include "Scene.h"
#include <string>
#include <map>
#include <functional>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

class SceneFactory {
public:
    // 定义一个函数类型，它返回 Scene*
    using SceneCreator = std::function<Scene*(const json& params)>;

    // 注册场景：把 "Level1" 和 创建Level1的函数 绑定起来
    static void Register(std::string typeName, SceneCreator creator) {
        getRegistry()[typeName] = creator;
    }

    // 创建场景：根据名字 new 一个对象出来
    static Scene* Create(std::string typeName, const json& params) {
        auto it = getRegistry().find(typeName);
        if (it != getRegistry().end()) {
            return it->second(params); // 调用 lambda 创建对象
        }
        std::cerr << "Error: Unknown Scene -> " << typeName << std::endl;
        return nullptr;
    }

    // 【新增】辅助类：利用构造函数自动注册
    struct Proxy {
        Proxy(std::string name, SceneCreator creator) {
            SceneFactory::Register(name, creator);
        }
    };

private:
    // 使用静态局部变量，解决静态初始化顺序问题 (C++ 单例惯用法)
    static std::map<std::string, SceneCreator>& getRegistry() {
        static std::map<std::string, SceneCreator> registry;
        return registry;
    }
};