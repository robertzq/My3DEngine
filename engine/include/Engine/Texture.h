#pragma once

// 引擎拥有的 GPU 纹理句柄；公开 API 不暴露 GLuint。
//
// ownership：
// * 由 ResourceManager / TextRenderer 的缓存持有（new 出来，DestroyTexture 释放）。
// * Entity / TileMap / Behavior 只持 non-owning Texture*。
// * 必须通过 Renderer::DestroyTexture 在 GL context 有效期间释放。
class Texture {
public:
    int Width() const { return width_; }
    int Height() const { return height_; }
    bool Valid() const { return id_ != 0; }

private:
    friend class Renderer;
    friend class RenderTarget;
    friend class PageCurl;
    unsigned int id_ = 0;
    int width_ = 0;
    int height_ = 0;
};
