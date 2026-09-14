#include "Engine/Entity.h"
#include "Engine/Renderer.h"
#include "Engine/SpriteEffect.h"

Entity::Entity() {
    animator.Bind(&sprite);
}
Entity::~Entity() = default;

SDL_Rect Entity::Bounds() const {
    SDL_Rect rect;
    rect.x = static_cast<int>(transform.x) + collider.offset.x;
    rect.y = static_cast<int>(transform.y) + collider.offset.y;
    rect.w = collider.offset.w > 0 ? collider.offset.w : transform.w;
    rect.h = collider.offset.h > 0 ? collider.offset.h : transform.h;
    return rect;
}

void Entity::SetPosition(float x, float y) {
    transform.x = x;
    transform.y = y;
}

void Entity::Render(const SDL_Rect& camera) const {
    if (!sprite.texture) return;

    SDL_Rect dest;
    dest.x = static_cast<int>(transform.x) - camera.x;
    dest.y = static_cast<int>(transform.y) - camera.y;
    dest.w = transform.w;
    dest.h = transform.h;

    SDL_Rect src = (sprite.src.w > 0 && sprite.src.h > 0) ? sprite.src : SDL_Rect{0, 0, 0, 0};

    SpriteDrawOptions options;
    options.effect = sprite.effect;
    options.flipX = (sprite.flip & SDL_FLIP_HORIZONTAL) != 0;
    options.flipY = (sprite.flip & SDL_FLIP_VERTICAL) != 0;
    Renderer::DrawSprite(sprite.texture, src, dest, options);
}
