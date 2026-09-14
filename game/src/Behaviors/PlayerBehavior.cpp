#include "Behaviors/PlayerBehavior.h"
#include "Engine/BehaviorRegistry.h"
#include "Engine/Entity.h"
#include "Engine/Input.h"
#include "Engine/Log.h"
#include "Engine/Physics.h"
#include "Engine/ResourceManager.h"
#include "Engine/SceneManager.h"

void PlayerBehavior::OnSpawn(SceneContext& context) {
    std::string texture = config.value("texture", "rpgPlayer.png");
    self->sprite.texture = ResourceManager::GetTexture(texture);

    frames = config.value("frames", 4);
    rows = config.value("rows", 4);
    speed = config.value("speed", 240.0f);
    gravity = config.value("gravity", 1800.0f);
    platformer = config.value("platformer", false);

    int texW = 0, texH = 0;
    if (self->sprite.texture) SDL_QueryTexture(self->sprite.texture, nullptr, nullptr, &texW, &texH);
    frameW = frames > 0 ? texW / frames : texW;
    frameH = rows > 0 ? texH / rows : texH;
    self->sprite.src = {0, 0, frameW, frameH};

    if (self->transform.w == 0) self->transform.w = config.value("width", 50);
    if (self->transform.h == 0) self->transform.h = config.value("height", 50);

    if (platformer) {
        self->collider.offset = {6, 4, self->transform.w - 12, self->transform.h - 4};
    } else {
        self->collider.offset = {15, 20, self->transform.w - 30, self->transform.h - 20};
    }
    self->collider.enabled = true;
}

void PlayerBehavior::HandleEvent(SceneContext& context, SDL_Event& event) {
    if (!platformer || !inputEnabled) return;
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE && onGround) {
        velY = -600.0f;
    }
}

void PlayerBehavior::Update(SceneContext& context, float deltaTime) {
    if (!inputEnabled) return;

    if (platformer) {
        int dx = 0;
        if (Input::IsKeyDown(SDL_SCANCODE_A)) dx -= 1;
        if (Input::IsKeyDown(SDL_SCANCODE_D)) dx += 1;
        velX = dx * speed;               // 像素/秒
        velY += gravity * deltaTime;     // 像素/秒

        float stepX = velX * deltaTime;
        float stepY = velY * deltaTime;
        self->transform.x += stepX;
        self->transform.y += stepY;
        if (context.map) Physics::MovePlatformer(*self, stepX, stepY, onGround, context.map->Colliders());
        // 落地 / 顶头后清零垂直速度（MovePlatformer 命中时会把 stepY 置 0）
        if (onGround || (velY < 0.0f && stepY == 0.0f)) velY = 0.0f;

        if (context.map) {
            float maxX = context.map->WidthPx() - self->transform.w;
            if (self->transform.x < 0) self->transform.x = 0;
            if (self->transform.y < 0) self->transform.y = 0;
            if (maxX > 0 && self->transform.x > maxX) self->transform.x = maxX;
        }

        if (velX < 0) self->sprite.flip = SDL_FLIP_HORIZONTAL;
        else if (velX > 0) self->sprite.flip = SDL_FLIP_NONE;

        Animate(dx != 0);
        return;
    }

    int dx = 0, dy = 0;
    if (Input::IsKeyDown(SDL_SCANCODE_A)) dx -= 1;
    if (Input::IsKeyDown(SDL_SCANCODE_D)) dx += 1;
    if (Input::IsKeyDown(SDL_SCANCODE_W)) dy -= 1;
    if (Input::IsKeyDown(SDL_SCANCODE_S)) dy += 1;

    velX = dx * speed * deltaTime;
    velY = dy * speed * deltaTime;

    self->transform.x += velX;
    self->transform.y += velY;
    if (context.map) Physics::MoveTopDown(*self, velX, velY, context.map->Colliders());

    if (context.map) {
        float maxX = context.map->WidthPx() - self->transform.w;
        float maxY = context.map->HeightPx() - self->transform.h;
        if (self->transform.x < 0) self->transform.x = 0;
        if (self->transform.y < 0) self->transform.y = 0;
        if (maxX > 0 && self->transform.x > maxX) self->transform.x = maxX;
        if (maxY > 0 && self->transform.y > maxY) self->transform.y = maxY;
    }

    if (velY > 0) direction = 0;
    else if (velY < 0) direction = 3;
    if (velX > 0) direction = 2;
    else if (velX < 0) direction = 1;
    self->sprite.src.y = direction * frameH;

    Animate(dx != 0 || dy != 0);
}

void PlayerBehavior::Animate(bool moving) {
    if (moving) {
        int frame = (static_cast<int>(SDL_GetTicks()) / 100) % frames;
        self->sprite.src.x = frameW * frame;
    } else {
        self->sprite.src.x = 0;
    }
}

static BehaviorRegistry::Proxy proxy_player("Player", []() {
    return new PlayerBehavior();
});
