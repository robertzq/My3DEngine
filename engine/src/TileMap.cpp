#include "Engine/TileMap.h"
#include "Engine/Config.h"
#include "Engine/Log.h"
#include "Engine/ResourceManager.h"
#include <sstream>

bool TileMap::Load(const std::string& mapResourceId, const TileSet& tileSet) {
    this->tileSet = tileSet;
    tileSize = tileSet.tileSize > 0 ? tileSet.tileSize : EngineConfig::TILE_SIZE;

    std::string content = ResourceManager::GetTextContent(mapResourceId);
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
            SDL_Texture* tex = ResourceManager::GetTexture(entry.second.texture);
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

void TileMap::DrawLayer(SDL_Renderer* renderer, const SDL_Rect& camera, bool overlayLayer) const {
    for (int row = 0; row < static_cast<int>(data.size()); ++row) {
        for (int col = 0; col < static_cast<int>(data[row].size()); ++col) {
            int id = data[row][col];
            if (tileSet.IsOverlay(id) != overlayLayer) continue;

            auto texIt = textures.find(id);
            if (texIt == textures.end()) continue;

            SDL_Rect dest = {col * tileSize - camera.x, row * tileSize - camera.y, tileSize, tileSize};
            if (dest.x < -tileSize || dest.x > camera.w || dest.y < -tileSize || dest.y > camera.h) continue;

            SDL_RenderCopy(renderer, texIt->second, nullptr, &dest);
        }
    }
}

void TileMap::Draw(SDL_Renderer* renderer, const SDL_Rect& camera) const {
    DrawGround(renderer, camera);
}

void TileMap::DrawGround(SDL_Renderer* renderer, const SDL_Rect& camera) const {
    DrawLayer(renderer, camera, false);
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

void TileMap::SetTile(int col, int row, int id) {
    if (row < 0 || row >= static_cast<int>(data.size())) return;
    if (col < 0 || col >= static_cast<int>(data[row].size())) return;
    data[row][col] = id;
    RebuildMetadata();
}
