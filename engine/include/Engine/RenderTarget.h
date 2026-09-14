#pragma once

class Texture;

// 离屏渲染目标（color-only FBO）。拥有自己的 color Texture。
// 不向 gameplay 暴露 GLuint / FBO id；生命周期由持有者（PostProcess）管理。
class RenderTarget {
public:
    RenderTarget() = default;
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    bool Create(int width, int height);
    bool Resize(int width, int height);   // 尺寸不变则 no-op；否则重建 attachment
    void Destroy();

    void Bind() const;     // 绑定 FBO 并把 viewport 设为自身尺寸
    void Unbind() const;   // 绑定默认 framebuffer

    bool Valid() const { return fbo_ != 0 && color_ != nullptr; }
    int Width() const { return width_; }
    int Height() const { return height_; }
    const Texture* ColorTexture() const { return color_; }

private:
    unsigned int fbo_ = 0;
    Texture* color_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};
