// src/Engine/Engine.cpp
#include "Engine.h"
#include <bgfx/platform.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include <cstdio>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>   // 只引入 QuartzCore 基础
#import <Metal/Metal.h>

bool Engine::init(int w, int h, const char* title) {
    m_width = w; m_height = h;

    if (!glfwInit()) { fprintf(stderr, "[ERR] glfwInit failed\n"); return false; }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!m_window) { fprintf(stderr, "[ERR] glfwCreateWindow failed\n"); glfwTerminate(); return false; }
    glfwShowWindow(m_window);
    glfwSetWindowPos(m_window, 120, 120);

    // —— 关键：把 NSView 的 layer 设成 CAMetalLayer，并传给 bgfx ——
    NSWindow* nswin = (NSWindow*)glfwGetCocoaWindow(m_window);
    if (!nswin) { fprintf(stderr, "[ERR] glfwGetCocoaWindow null\n"); return false; }
    NSView* view = [nswin contentView];
    [view setWantsLayer:YES];
    // —— 关键：不直接写 CAMetalLayer 类型名，避免需要头文件和符号 —— //
        Class CAMetalLayerClass = NSClassFromString(@"CAMetalLayer");
        if (!CAMetalLayerClass) {
            fprintf(stderr, "[ERR] CAMetalLayer class not found (Metal unsupported?)\n");
            return false;
        }
        if (![view.layer isKindOfClass:CAMetalLayerClass]) {
            id newLayer = [CAMetalLayerClass layer];
            [view setLayer:newLayer];
        }
        id metalLayer = view.layer;
        // 设置常用属性（用 KVC，避免直接用枚举/结构体类型）
        if ([metalLayer respondsToSelector:@selector(setFramebufferOnly:)]) {
            [metalLayer setValue:@(YES) forKey:@"framebufferOnly"];
        }
        if ([metalLayer respondsToSelector:@selector(setPixelFormat:)]) {
            // 80 == MTLPixelFormatBGRA8Unorm
            [metalLayer setValue:@(80) forKey:@"pixelFormat"];
        }
        // ★ 计算像素尺寸（注意用 backingScaleFactor）
        CGFloat scale = 1.0;
        if ([nswin respondsToSelector:@selector(backingScaleFactor)]) {
            scale = [nswin backingScaleFactor];
        }
        uint32_t fbw = (uint32_t)llround((double)w * (double)scale);
        uint32_t fbh = (uint32_t)llround((double)h * (double)scale);

        // ★ 同步 CAMetalLayer 的像素尺寸 & contentsScale
        if ([metalLayer respondsToSelector:@selector(setDrawableSize:)]) {
            CGSize size = CGSizeMake((CGFloat)fbw, (CGFloat)fbh);
            [metalLayer setValue:[NSValue valueWithSize:size] forKey:@"drawableSize"];
        }
        if ([metalLayer respondsToSelector:@selector(setContentsScale:)]) {
            [metalLayer setValue:@(scale) forKey:@"contentsScale"];
        }

    bgfx::PlatformData pd{};
    pd.nwh = (void*)metalLayer;   // Metal 要传 CAMetalLayer*
    pd.ndt = nullptr;
    pd.context = nullptr;
    pd.backBuffer = nullptr;
    pd.backBufferDS = nullptr;

    bgfx::Init init{};
    init.type = bgfx::RendererType::Metal;     // ← 用 Metal
    init.platformData = pd;
    init.resolution.width  = fbw;
    init.resolution.height = fbh;
    init.resolution.reset  = BGFX_RESET_VSYNC;

    fprintf(stderr, "[dbg] calling bgfx::init (Metal)\n");
    if (!bgfx::init(init)) {
        fprintf(stderr, "[ERR] bgfx::init failed\n");
        glfwDestroyWindow(m_window); m_window = nullptr;
        glfwTerminate();
        return false;
    }
    // ===== 在这里加 A 部分的代码 =====
    static const float viewMtx[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    static const float projMtx[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
   bgfx::setViewTransform(0, viewMtx, projMtx);
   bgfx::setViewRect(0, 0, 0, (uint16_t)fbw, (uint16_t)fbh);

   // 可选：打开统计面板便于排查是否提交了 draw
   bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS);

   // 清屏设置（你已设置，可保留）
   bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xE8F5E9FF, 1.0f, 0);

    m_inited = true;
    fprintf(stderr, "[dbg] Engine::init OK Metal %dx%d\n", w, h);
    return true;
}

void Engine::frameBegin() {
    glfwPollEvents();

        // ★ 每帧读取 framebuffer 像素大小
        int fbw=0, fbh=0;
        glfwGetFramebufferSize(m_window, &fbw, &fbh);

        static int lastW=-1, lastH=-1;
        if (fbw>0 && fbh>0 && (fbw!=lastW || fbh!=lastH)) {
            lastW = fbw; lastH = fbh;
            bgfx::reset((uint32_t)fbw, (uint32_t)fbh, BGFX_RESET_VSYNC);
            bgfx::setViewRect(0, 0, 0, (uint16_t)fbw, (uint16_t)fbh);
        }

        bgfx::touch(0);
        bgfx::dbgTextClear();
        bgfx::dbgTextPrintf(0, 0, 0x0F, "My3DEngine running...");
        bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS);
}

void Engine::frameEnd() {
    bgfx::frame();
}

bool Engine::shouldClose() const {
    // 如果窗口没建成功，直接返回 true，避免死循环
    if (!m_window) return true;
    return glfwWindowShouldClose(m_window);
}

void Engine::shutdown() {
    if (m_inited) {
        bgfx::shutdown();
        m_inited = false;
    }
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}
