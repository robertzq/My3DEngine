// src/Engine/Engine.h
#pragma once
#include <bgfx/bgfx.h>
#include <GLFW/glfw3.h>

class Engine {
public:
    bool init(int w, int h, const char* title);
    void shutdown();

    void frameBegin();
    void frameEnd();

    bool shouldClose() const;
    GLFWwindow* window() const { return m_window; }
    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    GLFWwindow* m_window = nullptr;
    int m_width = 0, m_height = 0;
    bool m_inited = false;
};
