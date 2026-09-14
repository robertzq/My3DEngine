#include "Behaviors/PlayerBehavior.h"
#include "Engine/BehaviorRegistry.h"
#include "Engine/Entity.h"
#include "Engine/Input.h"
#include "Engine/Log.h"
#include "Engine/Physics.h"
#include "Engine/ResourceManager.h"
#include "Engine/SceneManager.h"
#include "Engine/SpriteEffect.h"
#include "Engine/Texture.h"

void PlayerBehavior::OnSpawn(SceneContext& context) {
    std::string texture = config.value("texture", "player_sheet.png");
    self->sprite.texture = ResourceManager::GetTexture(texture);

    cols = config.value("cols", 6);
    rows = config.value("rows", 5);
    speed = config.value("speed", 180.0f);

    int texW = self->sprite.texture ? self->sprite.texture->Width() : 0;
    int texH = self->sprite.texture ? self->sprite.texture->Height() : 0;
    frameW = cols > 0 ? texW / cols : texW;
    frameH = rows > 0 ? texH / rows : texH;

    if (self->transform.w == 0) self->transform.w = frameW;
    if (self->transform.h == 0) self->transform.h = frameH;

    self->animator.SetSheet(frameW, frameH, cols);
    if (config.contains("animations")) self->animator.LoadClips(config["animations"]);
    self->sprite.src = {0, 0, frameW, frameH};
    self->collider.offset = {12, 28, self->transform.w - 24, self->transform.h - 28};
    self->collider.enabled = true;
    self->collider.layer = Layers::Player;
    self->collider.mask = Layers::World | Layers::Enemy | Layers::Trigger;

    self->transform.x -= self->transform.w / 2.0f;
    self->transform.y -= self->transform.h / 2.0f;
}

void PlayerBehavior::Update(SceneContext& context, float deltaTime) {
    int dx = 0, dy = 0;
    if (Input::Down("MoveLeft")) dx -= 1;
    if (Input::Down("MoveRight")) dx += 1;
    if (Input::Down("MoveUp")) dy -= 1;
    if (Input::Down("MoveDown")) dy += 1;

    // speed 为像素/秒，乘以 dt 得到本帧位移（与帧率无关）
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

    if (dx < 0) self->animator.Play("walk_left");
    else if (dx > 0) self->animator.Play("walk_right");
    else if (dy < 0) self->animator.Play("walk_up");
    else if (dy > 0) self->animator.Play("walk_down");
    else self->animator.Play("idle");
    self->animator.Update(deltaTime);

    // 数据驱动 sprite shader demo：按交互键触发 hit flash，参数随 time 衰减
    if (Input::Pressed("Interact")) flash = 1.0f;
    if (flash > 0.0f) {
        flash -= deltaTime * 2.5f;
        if (flash < 0.0f) flash = 0.0f;
    }
    if (self->sprite.effect) self->sprite.effect->SetFloat("u_intensity", flash);
}

static BehaviorRegistry::Proxy proxy_player("Player", []() {
    return new PlayerBehavior();
});
