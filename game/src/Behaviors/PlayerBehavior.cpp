#include "Behaviors/PlayerBehavior.h"
#include "Engine/BehaviorRegistry.h"
#include "Engine/Entity.h"
#include "Engine/Input.h"
#include "Engine/Log.h"
#include "Engine/Physics.h"
#include "Engine/ResourceManager.h"
#include "Engine/SceneManager.h"

void PlayerBehavior::OnSpawn(SceneContext& context) {
    std::string texture = config.value("texture", "player_sheet.png");
    self->sprite.texture = ResourceManager::GetTexture(texture);

    cols = config.value("cols", 6);
    rows = config.value("rows", 5);
    speed = config.value("speed", 3.0f);

    rowIdle = config.value("row_idle", 0);
    rowDown = config.value("row_down", 1);
    rowUp = config.value("row_up", 2);
    rowLeft = config.value("row_left", 3);
    rowRight = config.value("row_right", 4);
    currentRow = rowIdle;

    int texW = 0, texH = 0;
    if (self->sprite.texture) SDL_QueryTexture(self->sprite.texture, nullptr, nullptr, &texW, &texH);
    frameW = cols > 0 ? texW / cols : texW;
    frameH = rows > 0 ? texH / rows : texH;

    if (self->transform.w == 0) self->transform.w = frameW;
    if (self->transform.h == 0) self->transform.h = frameH;

    self->sprite.src = {0, currentRow * frameH, frameW, frameH};
    self->collider.offset = {12, 28, self->transform.w - 24, self->transform.h - 28};
    self->collider.enabled = true;

    self->transform.x -= self->transform.w / 2.0f;
    self->transform.y -= self->transform.h / 2.0f;
}

void PlayerBehavior::SelectRow(int dx, int dy) {
    if (dx < 0) currentRow = rowLeft;
    else if (dx > 0) currentRow = rowRight;
    else if (dy < 0) currentRow = rowUp;
    else if (dy > 0) currentRow = rowDown;
    else currentRow = rowIdle;
}

void PlayerBehavior::Animate(bool moving) {
    int frame = 0;
    if (moving && cols > 0) {
        frame = (static_cast<int>(SDL_GetTicks()) / 120) % cols;
    }
    self->sprite.src.x = frame * frameW;
    self->sprite.src.y = currentRow * frameH;
}

void PlayerBehavior::Update(SceneContext& context, float deltaTime) {
    int dx = 0, dy = 0;
    if (Input::IsKeyDown(SDL_SCANCODE_A)) dx -= 1;
    if (Input::IsKeyDown(SDL_SCANCODE_D)) dx += 1;
    if (Input::IsKeyDown(SDL_SCANCODE_W)) dy -= 1;
    if (Input::IsKeyDown(SDL_SCANCODE_S)) dy += 1;

    velX = dx * speed;
    velY = dy * speed;

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

    SelectRow(dx, dy);
    Animate(dx != 0 || dy != 0);
}

static BehaviorRegistry::Proxy proxy_player("Player", []() {
    return new PlayerBehavior();
});
