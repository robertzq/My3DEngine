#include "Engine/Physics.h"
#include <cmath> // abs

bool Physics::CheckCollision(const SDL_Rect& a, const SDL_Rect& b) {
    return SDL_HasIntersection(&a, &b);
}

bool Physics::ShouldCollide(const Collider& a, const Collider& b) {
    return (a.mask & b.layer) != 0 && (b.mask & a.layer) != 0;
}

bool Physics::Overlap(const Entity& a, const Entity& b) {
    if (!a.collider.enabled || !b.collider.enabled) return false;
    if (!ShouldCollide(a.collider, b.collider)) return false;

    SDL_Rect ra = a.Bounds();
    SDL_Rect rb = b.Bounds();
    return SDL_HasIntersection(&ra, &rb);
}

bool Physics::Overlap(const Entity& entity, const SDL_Rect& rect) {
    if (!entity.collider.enabled) return false;

    SDL_Rect bounds = entity.Bounds();
    return SDL_HasIntersection(&bounds, &rect);
}

void Physics::MoveTopDown(Entity& entity, float stepX, float stepY,
                          const std::vector<SDL_Rect>& obstacles) {
    SDL_Rect bounds = entity.Bounds();

    for (const auto& obstacle : obstacles) {
        if (!SDL_HasIntersection(&bounds, &obstacle)) continue;

        SDL_Rect xBack = bounds;
        xBack.x -= static_cast<int>(stepX);
        if (!SDL_HasIntersection(&xBack, &obstacle)) {
            if (stepX > 0) entity.transform.x = obstacle.x - entity.collider.offset.x - bounds.w;
            else if (stepX < 0) entity.transform.x = obstacle.x + obstacle.w - entity.collider.offset.x;
            bounds = entity.Bounds();
            continue;
        }

        SDL_Rect yBack = bounds;
        yBack.y -= static_cast<int>(stepY);
        if (!SDL_HasIntersection(&yBack, &obstacle)) {
            if (stepY > 0) entity.transform.y = obstacle.y - entity.collider.offset.y - bounds.h;
            else if (stepY < 0) entity.transform.y = obstacle.y + obstacle.h - entity.collider.offset.y;
            bounds = entity.Bounds();
        }
    }
}

void Physics::MovePlatformer(Entity& entity, float stepX, float& stepY, bool& onGround,
                             const std::vector<SDL_Rect>& obstacles) {
    onGround = false;
    SDL_Rect bounds = entity.Bounds();

    for (const auto& obstacle : obstacles) {
        SDL_Rect overlap;
        if (!SDL_IntersectRect(&bounds, &obstacle, &overlap)) continue;

        bool vertical = overlap.w > overlap.h;
        if (vertical) {
            if (stepY >= 0 && bounds.y < obstacle.y) {
                entity.transform.y = obstacle.y - entity.collider.offset.y - bounds.h;
                stepY = 0;
                onGround = true;
            } else if (stepY < 0) {
                entity.transform.y = obstacle.y + obstacle.h - entity.collider.offset.y;
                stepY = 0;
            }
        } else {
            int footOverlap = (bounds.y + bounds.h) - obstacle.y;
            if (footOverlap >= 8) {
                if (stepX > 0) entity.transform.x = obstacle.x - entity.collider.offset.x - bounds.w;
                else if (stepX < 0) entity.transform.x = obstacle.x + obstacle.w - entity.collider.offset.x;
            }
        }
        bounds = entity.Bounds();
    }
}
