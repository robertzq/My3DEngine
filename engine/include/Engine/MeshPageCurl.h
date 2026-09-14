#pragma once

class Texture;

// Mesh-based Bézier Page Curl（方案 C）：CPU 对 N×N 网格做真实翻折变形，
// fragment 负责 front/backside + fold 光照 + contact shadow。
// 与旧的 analytic PageCurl 并存，作为正式实现。
struct MeshPageCurlParams {
    float progress = 0.0f;   // 0..1

    // 抓取/拖动语义（归一化 0..1 页面坐标）
    float originX = 1.0f, originY = 1.0f;     // 被掀起的角
    float dragX = 1.0f, dragY = 1.0f;         // 当前拖动位置
    float control1X = 0.5f, control1Y = 0.5f; // fold ridge 弯曲趋势
    float control2X = 0.5f, control2Y = 0.5f;
    bool explicitControl = false;

    float curlRadius = 0.14f;   // 相对 min(W,H)
    float curvature = 0.8f;
    float shadowStrength = 0.7f;
    float highlightStrength = 0.35f;
    float backsideDarken = 0.18f;
    float edgeSoftness = 1.0f;

    int tessellation = 64;      // 网格细分
};

class MeshPageCurl {
public:
    static bool Init();
    static void Clean();
    static void Render(const Texture* pageTexture, const Texture* backTexture,
                       int width, int height, const MeshPageCurlParams& params);
};
