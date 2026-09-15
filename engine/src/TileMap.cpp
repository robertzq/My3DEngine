#include "Engine/TileMap.h"
#include "Engine/Config.h"
#include "Engine/Log.h"
#include <cstdio>
#include "Engine/Renderer.h"
#include "Engine/ResourceManager.h"
#include "Engine/ShaderManager.h"
#include "Engine/Shader.h"
#include <sstream>

bool TileMap::Load(const std::string& mapResourceId, const TileSet& tileSet) {
    this->tileSet = tileSet;
    tileSize = tileSet.tileSize > 0 ? tileSet.tileSize : EngineConfig::TILE_SIZE;

    std::string content = ResourceManager::GetText(mapResourceId);
    if (content.empty()) {
        LOG_ERROR("TileMap 加载失败，资源为空: " << mapResourceId);
        return false;
    }

    data.clear();
    width = 0;

    std::stringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty()) continue;

        std::vector<int> row;
        bool spaceSeparated = line.find(' ') != std::string::npos || line.find('\t') != std::string::npos;
        if (spaceSeparated) {
            std::stringstream ss(line);
            int value;
            while (ss >> value) row.push_back(value);
        } else {
            for (char c : line) {
                if (c >= '0' && c <= '9') row.push_back(c - '0');
            }
        }

        if (!row.empty()) {
            if (static_cast<int>(row.size()) > width) width = static_cast<int>(row.size());
            data.push_back(row);
        }
    }
    height = static_cast<int>(data.size());

    textures.clear();
    for (const auto& entry : tileSet.tiles) {
        if (!entry.second.texture.empty()) {
            Texture* tex = ResourceManager::GetTexture(entry.second.texture);
            if (tex) textures[entry.first] = tex;
        }
    }

    RebuildMetadata();
    LOG_INFO("TileMap 加载成功: " << mapResourceId << " (" << width << "x" << height << ")");
    return true;
}

void TileMap::RebuildMetadata() {
    colliders.clear();
    triggers.clear();
    overlays.clear();

    for (int row = 0; row < static_cast<int>(data.size()); ++row) {
        for (int col = 0; col < static_cast<int>(data[row].size()); ++col) {
            int id = data[row][col];
            SDL_Rect rect = {col * tileSize, row * tileSize, tileSize, tileSize};

            if (tileSet.IsSolid(id)) colliders.push_back(rect);

            std::string trigger = tileSet.TriggerOf(id);
            if (!trigger.empty()) triggers.emplace_back(trigger, rect);

            if (tileSet.IsOverlay(id)) {
                auto texIt = textures.find(id);
                if (texIt != textures.end()) {
                    overlays.push_back({rect, texIt->second, rect.y + rect.h});
                }
            }
        }
    }
}

void TileMap::DrawLayer(const SDL_Rect& camera, bool overlayLayer) const {
    for (int row = 0; row < static_cast<int>(data.size()); ++row) {
        for (int col = 0; col < static_cast<int>(data[row].size()); ++col) {
            int id = data[row][col];
            if (tileSet.IsOverlay(id) != overlayLayer) continue;

            auto texIt = textures.find(id);
            if (texIt == textures.end()) continue;

            SDL_Rect dest = {col * tileSize - camera.x, row * tileSize - camera.y, tileSize, tileSize};
            if (dest.x < -tileSize || dest.x > camera.w || dest.y < -tileSize || dest.y > camera.h) continue;

            Renderer::DrawSprite(texIt->second, SDL_Rect{0, 0, 0, 0}, dest, SDL_FLIP_NONE);
        }
    }
}

void TileMap::Draw(const SDL_Rect& camera) const {
    DrawGround(camera);
}

void TileMap::DrawGround(const SDL_Rect& camera) const {
    DrawLayer(camera, false);
}

std::vector<SDL_Rect> TileMap::TilesWithId(int id) const {
    std::vector<SDL_Rect> result;
    for (int row = 0; row < static_cast<int>(data.size()); ++row) {
        for (int col = 0; col < static_cast<int>(data[row].size()); ++col) {
            if (data[row][col] == id) {
                result.push_back({col * tileSize, row * tileSize, tileSize, tileSize});
            }
        }
    }
    return result;
}

std::vector<SDL_Rect> TileMap::TriggerRects(const std::string& name) const {
    std::vector<SDL_Rect> result;
    for (const auto& entry : triggers) {
        if (entry.first == name) result.push_back(entry.second);
    }
    return result;
}

// 斜等距 2:1 底面。
// 思路：世界逻辑格中心经 Projection::ToScreen 投影到屏幕，
// 再用菱形几何（半宽 W/2、半高 H/2）构造四顶点，用 DrawMesh 带 uv 贴图。
// 这样地面呈现仙剑98式的纵深菱形铺装，而不改变逻辑网格数据/碰撞。
void TileMap::DrawGroundIso(const Projection& proj, const SDL_Rect& camera) const {
    if (data.empty() || tileSize <= 0) return;
    const float s = (proj.mode == ProjectionMode::Iso) ? proj.view.scale : 1.0f;
    const int hw = static_cast<int>(proj.grid.halfW() * s);   // 屏幕菱形半宽
    const int hh = static_cast<int>(proj.grid.halfH() * s);   // 屏幕菱形半高
    const int mapCull = hw + hh + tileSize;

    for (int row = 0; row < static_cast<int>(data.size()); ++row) {
        for (int col = 0; col < static_cast<int>(data[row].size()); ++col) {
            int id = data[row][col];
            if (tileSet.IsOverlay(id)) continue;   // overlay 交给 DrawWorld 统一排序

            auto texIt = textures.find(id);
            if (texIt == textures.end()) continue;
            const Texture* tex = texIt->second;

            // 世界格（左下角原点化换算）：这里以格中心为锚。
            float wx = static_cast<float>(col * tileSize + tileSize / 2);
            float wy = static_cast<float>(row * tileSize + tileSize / 2);
            SDL_Point c = proj.WorldToScreen(wx, wy);
            c.x -= camera.x;
            c.y -= camera.y;

            if (c.x + mapCull < 0 || c.x - mapCull > camera.w ||
                c.y + mapCull < 0 || c.y - mapCull > camera.h) continue;

            // 菱形四顶点（screen，相对中心）
            SDL_Point pts[4];
            pts[0] = { c.x - hw, c.y };         // 左
            pts[1] = { c.x, c.y - hh };         // 上
            pts[2] = { c.x + hw, c.y };         // 右
            pts[3] = { c.x, c.y + hh };         // 下

            // uv：左上=左顶点(0.5,0)，上=上顶点(0,0.5)，右=右顶点(0.5,1)，下=下顶点(1,0.5)
            // 用逆时针顶/左/底/右的顺序构造，确保纹理朝向正确。
            MeshVertex verts[4] = {
                { (float)pts[0].x, (float)pts[0].y, 0.0f, 0.5f },
                { (float)pts[1].x, (float)pts[1].y, 0.5f, 0.0f },
                { (float)pts[2].x, (float)pts[2].y, 1.0f, 0.5f },
                { (float)pts[3].x, (float)pts[3].y, 0.5f, 1.0f },
            };
            unsigned short idx[6] = {0, 1, 2, 0, 2, 3};

            // 使用默认 sprite shader 的合成解析：ShaderManager 已注册 sprite_default。
            // 但 DrawMesh 需要显式 Shader&，这里取默认 sprite shader。
            // TileSet 纹理是方形；等距地面把方形裁到菱形内（菱形外 alpha=0）。
            Shader* shader = ShaderManager::Get("mesh_default");
            if (shader) Renderer::DrawMesh(*shader, verts, 4, idx, 6, tex, MeshDrawOptions{});
        }
    }



}

void TileMap::SetTile(int col, int row, int id) {
    if (row < 0 || row >= static_cast<int>(data.size())) return;
    if (col < 0 || col >= static_cast<int>(data[row].size())) return;
    data[row][col] = id;
    RebuildMetadata();
}
