#pragma once
#include <string>

class Texture;

// 贝塞尔 Page Curl 转场参数（几何 + 光照）。第一版自动翻页。
struct PageCurlParams {
    float progress = 0.0f;          // 0..1

    // 几何（归一化 0..1 页面坐标）
    float originX = 1.0f, originY = 1.0f;   // 翻页起点（被掀起的角）
    float dragX = 1.0f, dragY = 1.0f;       // 拖拽点（影响 fold 位置/形态）
    float control1X = 0.5f, control1Y = 0.5f; // 贝塞尔控制点 1（真实参与折痕）
    float control2X = 0.5f, control2Y = 0.5f; // 贝塞尔控制点 2
    bool explicitControl = false;             // true 时用上面的控制点，否则按 curvature 推导

    float curlRadius = 0.14f;       // 相对 min(W,H)
    float curvature = 0.8f;
    float shadowStrength = 0.7f;
    float highlightStrength = 0.35f;
    float backsideDarken = 0.18f;
    float edgeSoftness = 1.0f;
};

// 屏幕空间 Page Curl：把 page 纹理（旧场景）按贝塞尔折痕卷起，露出 back 纹理（新场景）。
// 使用 fragment analytic curl（Step 3 允许的方案 B）。
class PageCurl {
public:
    static bool Init();
    static void Clean();
    static bool Ready();
    static void Render(const Texture* pageTexture, const Texture* backTexture,
                       int width, int height, const PageCurlParams& params);
};
