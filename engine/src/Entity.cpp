#include "Engine/Entity.h"

Entity::Entity() = default;
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

void Entity::Render(SDL_Renderer* renderer, const SDL_Rect& camera) const {
    if (!sprite.texture) return;

    SDL_Rect dest;
    dest.x = static_cast<int>(transform.x) - camera.x;
    dest.y = static_cast<int>(transform.y) - camera.y;
    dest.w = transform.w;
    dest.h = transform.h;

    const SDL_Rect* src = (sprite.src.w > 0 && sprite.src.h > 0) ? &sprite.src : nullptr;
    SDL_RenderCopyEx(renderer, sprite.texture, src, &dest, 0.0, nullptr, sprite.flip);
}
