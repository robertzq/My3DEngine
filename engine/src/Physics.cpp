#include "Engine/Physics.h"
#include <cmath> // abs

bool Physics::CheckCollision(const SDL_Rect& a, const SDL_Rect& b) {
    return SDL_HasIntersection(&a, &b);
}

void Physics::MoveTopDown(Entity& entity, float velX, float velY, const std::vector<SDL_Rect>& obstacles) {
    SDL_Rect bounds = entity.Bounds();

    for (const auto& obstacle : obstacles) {
        if (!SDL_HasIntersection(&bounds, &obstacle)) continue;

        SDL_Rect xBack = bounds;
        xBack.x -= static_cast<int>(velX);
        if (!SDL_HasIntersection(&xBack, &obstacle)) {
            if (velX > 0) entity.transform.x = obstacle.x - entity.collider.offset.x - bounds.w;
            else if (velX < 0) entity.transform.x = obstacle.x + obstacle.w - entity.collider.offset.x;
            bounds = entity.Bounds();
            continue;
        }

        SDL_Rect yBack = bounds;
        yBack.y -= static_cast<int>(velY);
        if (!SDL_HasIntersection(&yBack, &obstacle)) {
            if (velY > 0) entity.transform.y = obstacle.y - entity.collider.offset.y - bounds.h;
            else if (velY < 0) entity.transform.y = obstacle.y + obstacle.h - entity.collider.offset.y;
            bounds = entity.Bounds();
        }
    }
}

void Physics::MovePlatformer(Entity& entity, float velX, float& velY, bool& onGround,
                             const std::vector<SDL_Rect>& obstacles) {
    onGround = false;
    SDL_Rect bounds = entity.Bounds();

    for (const auto& obstacle : obstacles) {
        SDL_Rect overlap;
        if (!SDL_IntersectRect(&bounds, &obstacle, &overlap)) continue;

        bool vertical = overlap.w > overlap.h;
        if (vertical) {
            if (velY >= 0 && bounds.y < obstacle.y) {
                entity.transform.y = obstacle.y - entity.collider.offset.y - bounds.h;
                velY = 0;
                onGround = true;
            } else if (velY < 0) {
                entity.transform.y = obstacle.y + obstacle.h - entity.collider.offset.y;
                velY = 0;
            }
        } else {
            int footOverlap = (bounds.y + bounds.h) - obstacle.y;
            if (footOverlap >= 8) {
                if (velX > 0) entity.transform.x = obstacle.x - entity.collider.offset.x - bounds.w;
                else if (velX < 0) entity.transform.x = obstacle.x + obstacle.w - entity.collider.offset.x;
            }
        }
        bounds = entity.Bounds();
    }
}