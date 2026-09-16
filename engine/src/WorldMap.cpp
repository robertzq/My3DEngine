#include "Engine/WorldMap.h"
#include "Engine/TileMap.h"   // TileSprite 完整定义
#include "Engine/Texture.h"
#include "Engine/Renderer.h"
#include "Engine/ShaderManager.h"
#include "Engine/Shader.h"
#include "Engine/Config.h"
#include "Engine/ResourceManager.h"
#include "Engine/Log.h"
#include <sstream>

bool WorldMap::LoadLegacyMap(const std::string& mapResourceId, const TileSet& tileSet) {
    tileSet_ = tileSet;
    tileSize_ = tileSet.tileSize > 0 ? tileSet.tileSize : EngineConfig::TILE_SIZE;

    std::string content = ResourceManager::GetText(mapResourceId);
    if (content.empty()) {
        LOG_ERROR("WorldMap 加载失败，资源为空: " << mapResourceId);
        return false;
    }

    // 解析旧单矩阵：行 -> 数据扁平化进第 0 层。
    std::vector<int> flat;
    int width = 0;
    int height = 0;
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
            flat.insert(flat.end(), row.begin(), row.end());
            ++height;
        }
    }

    if (width <= 0 || height <= 0) {
        LOG_ERROR("WorldMap 加载失败，无有效地图数据: " << mapResourceId);
        return false;
    }

    // 丢弃多余行（与旧 TileMap 行为一致：data[y][x]，width 取最大行宽）。
    flat.resize(static_cast<size_t>(width) * height, 0);

    layers_.clear();
    layers_.emplace_back(width, height, TileLayer::kEmpty);
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            layers_[0].SetTile(col, row,
                flat[static_cast<size_t>(row) * width + col]);
        }
    }

    BuildTextures();
    RebuildMetadata();

    LOG_INFO("WorldMap 加载成功: " << mapResourceId << " (" << width << "x" << height << ")");
    return true;
}

int WorldMap::AddLayer(int width, int height, int fill) {
    layers_.emplace_back(width, height, fill);
    RebuildMetadata();
    return static_cast<int>(layers_.size() - 1);
}

TileLayer* WorldMap::LayerAt(size_t index) {
    return index < layers_.size() ? &layers_[index] : nullptr;
}

const TileLayer* WorldMap::LayerAt(size_t index) const {
    return index < layers_.size() ? &layers_[index] : nullptr;
}

std::vector<std::pair<int, int>> WorldMap::QueryCell(int col, int row) const {
    std::vector<std::pair<int, int>> result;
    for (size_t i = 0; i < layers_.size(); ++i) {
        if (i >= layers_.size()) break;
        int tile = layers_[i].GetTile(col, row);
        if (tile != TileLayer::kEmpty && tile != TileLayer::kInvalid) {
            result.emplace_back(static_cast<int>(i), tile);
        }
    }
    return result;
}

int WorldMap::GetTile(int layerIndex, int col, int row) const {
    if (layerIndex < 0 || static_cast<size_t>(layerIndex) >= layers_.size()) return TileLayer::kInvalid;
    return layers_[layerIndex].GetTile(col, row);
}

bool WorldMap::SetTile(int layerIndex, int col, int row, int id) {
    if (layerIndex < 0 || static_cast<size_t>(layerIndex) >= layers_.size()) return false;
    return layers_[layerIndex].SetTile(col, row, id);
}

int WorldMap::GetGroundTile(int col, int row) const {
    return layers_.empty() ? TileLayer::kInvalid : layers_[0].GetTile(col, row);
}

bool WorldMap::SetGroundTile(int col, int row, int id) {
    if (layers_.empty()) return false;
    return layers_[0].SetTile(col, row, id);
}

void WorldMap::BuildTextures() {
    textures_.clear();
    for (const auto& entry : tileSet_.tiles) {
        if (!entry.second.texture.empty()) {
            Texture* tex = ResourceManager::GetTexture(entry.second.texture);
            if (tex) textures_[entry.first] = tex;
        }
    }
}

void WorldMap::RebuildMetadata() {
    colliders_.clear();
    overlays_.clear();
    triggerNames_.clear();
    triggerRects_.clear();

    for (size_t i = 0; i < layers_.size(); ++i) {
        layers_[i].RebuildMetadata(tileSet_, tileSize_, &textures_);

        // 聚合 colliders
        const auto& lc = layers_[i].Colliders();
        colliders_.insert(colliders_.end(), lc.begin(), lc.end());

        // 聚合 overlays
        const auto& lo = layers_[i].Overlays();
        overlays_.insert(overlays_.end(), lo.begin(), lo.end());
    }
}

std::vector<SDL_Rect> WorldMap::TriggerRects(const std::string& name) const {
    std::vector<SDL_Rect> result;
    for (const auto& layer : layers_) {
        auto r = layer.TriggerRects(name);
        result.insert(result.end(), r.begin(), r.end());
    }
    return result;
}

std::vector<SDL_Rect> WorldMap::TilesWithId(int id) const {
    std::vector<SDL_Rect> result;
    for (const auto& layer : layers_) {
        int col = 0, row = 0;
        // 简单线性扫描该层
        for (row = 0; row < layer.Height(); ++row) {
            for (col = 0; col < layer.Width(); ++col) {
                if (layer.GetTile(col, row) == id) {
                    result.push_back({col * tileSize_, row * tileSize_, tileSize_, tileSize_});
                }
            }
        }
    }
    return result;
}

void WorldMap::DrawGround(const SDL_Rect& camera) const {
    // ground pass：画所有层的非 overlay 格，按层序先下后上。
    for (const auto& layer : layers_) {
        for (int row = 0; row < layer.Height(); ++row) {
            for (int col = 0; col < layer.Width(); ++col) {
                int id = layer.GetTile(col, row);
                if (tileSet_.IsOverlay(id)) continue;
                auto tIt = textures_.find(id);
                if (tIt == textures_.end()) continue;
                Texture* tex = tIt->second;
                SDL_Rect dest = {col * tileSize_ - camera.x, row * tileSize_ - camera.y, tileSize_, tileSize_};
                if (dest.x < -tileSize_ || dest.x > camera.w || dest.y < -tileSize_ || dest.y > camera.h) continue;
                Renderer::DrawSprite(tex, SDL_Rect{0, 0, 0, 0}, dest, SDL_FLIP_NONE);
            }
        }
    }
}

void WorldMap::DrawGroundIso(const Projection& proj, const SDL_Rect& camera) const {
    if (layers_.empty() || tileSize_ <= 0) return;
    const float s = (proj.mode == ProjectionMode::Iso) ? proj.view.scale : 1.0f;
    const int hw = static_cast<int>(proj.grid.halfW() * s);   // 屏幕菱形半宽
    const int hh = static_cast<int>(proj.grid.halfH() * s);   // 屏幕菱形半高
    const int mapCull = hw + hh + tileSize_;

    for (const auto& layer : layers_) {
        for (int row = 0; row < layer.Height(); ++row) {
            for (int col = 0; col < layer.Width(); ++col) {
                int id = layer.GetTile(col, row);
                if (tileSet_.IsOverlay(id)) continue;   // overlay 交给上层统一排序

                auto texIt = textures_.find(id);
                if (texIt == textures_.end()) continue;
                const Texture* tex = texIt->second;

                float wx = static_cast<float>(col * tileSize_ + tileSize_ / 2);
                float wy = static_cast<float>(row * tileSize_ + tileSize_ / 2);
                SDL_Point c = proj.WorldToScreen(wx, wy);
                c.x -= camera.x;
                c.y -= camera.y;

                if (c.x + mapCull < 0 || c.x - mapCull > camera.w ||
                    c.y + mapCull < 0 || c.y - mapCull > camera.h) continue;

                SDL_Point pts[4];
                pts[0] = { c.x - hw, c.y };
                pts[1] = { c.x, c.y - hh };
                pts[2] = { c.x + hw, c.y };
                pts[3] = { c.x, c.y + hh };

                MeshVertex verts[4] = {
                    { (float)pts[0].x, (float)pts[0].y, 0.0f, 0.5f },
                    { (float)pts[1].x, (float)pts[1].y, 0.5f, 0.0f },
                    { (float)pts[2].x, (float)pts[2].y, 1.0f, 0.5f },
                    { (float)pts[3].x, (float)pts[3].y, 0.5f, 1.0f },
                };
                unsigned short idx[6] = {0, 1, 2, 0, 2, 3};
                Shader* shader = ShaderManager::Get("mesh_default");
                if (shader) Renderer::DrawMesh(*shader, verts, 4, idx, 6, tex, MeshDrawOptions{});
            }
        }
    }
}