#include "Engine/RenderTarget.h"
#include "Engine/GL.h"
#include "Engine/Renderer.h"
#include "Engine/Texture.h"
#include "Engine/Log.h"

RenderTarget::~RenderTarget() { Destroy(); }

bool RenderTarget::Create(int width, int height) {
    Destroy();
    if (width <= 0 || height <= 0) return false;

    color_ = Renderer::CreateRenderTexture(width, height);
    if (!color_) {
        LOG_ERROR("RenderTarget: 创建 color texture 失败 " << width << "x" << height);
        return false;
    }

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_->id_, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("RenderTarget: framebuffer 不完整 (status=0x" << std::hex << status << std::dec
                  << ") " << width << "x" << height);
        Destroy();
        return false;
    }

    width_ = width;
    height_ = height;
    return true;
}

bool RenderTarget::Resize(int width, int height) {
    if (Valid() && width == width_ && height == height_) return true;
    return Create(width, height);
}

void RenderTarget::Destroy() {
    if (fbo_) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    if (color_) {
        Renderer::DestroyTexture(color_);
        color_ = nullptr;
    }
    width_ = 0;
    height_ = 0;
}

void RenderTarget::Bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
}

void RenderTarget::Unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
