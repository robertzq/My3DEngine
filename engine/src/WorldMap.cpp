#include "Engine/WorldMap.h"
#include "Engine/TileMap.h"   // TileSprite 完整定义
#include "Engine/Texture.h"
#include "Engine/Renderer.h"
#include "Engine/ShaderManager.h"
#include "Engine/Shader.h"
#include "Engine/Config.h"
#include "Engine/ResourceManager.h"
#include "Engine/Log.h"
#include <algorithm>
#include <sstream>

// 解析单个地图文件为宽/高 + 平铺数据（row-major）。
// 兼容空格/制表分隔 与 紧凑数字串 两种矩阵写法（对齐旧 TileMap::Load）。
// 返回 false 表示内容为空 / 无有效数据 / 列宽不确定（malformed）。
static bool ParseMapFile(const std::string& content,
                         int& outWidth, int& outHeight, std::vector<int>& outFlat) {
    outWidth = 0;
    outHeight = 0;
    outFlat.clear();

    std::stringstream stream(content);
    std::string line;
    std::vector<int> rowWidths;
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
            rowWidths.push_back(static_cast<int>(row.size()));
            if (static_cast<int>(row.size()) > outWidth) outWidth = static_cast<int>(row.size());
            outFlat.insert(outFlat.end(), row.begin(), row.end());
            ++outHeight;
        }
    }

    if (outWidth <= 0 || outHeight <= 0) return false;
    outFlat.resize(static_cast<size_t>(outWidth) * outHeight, 0);
    return true;
}

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
    if (!ParseMapFile(content, width, height, flat)) {
        LOG_ERROR("WorldMap 加载失败，无有效地图数据: " << mapResourceId);
        return false;
    }

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

bool WorldMap::LoadLayered(const std::vector<MapLayerSpec>& specs, const TileSet& tileSet) {
    tileSet_ = tileSet;
    tileSize_ = tileSet.tileSize > 0 ? tileSet.tileSize : EngineConfig::TILE_SIZE;

    if (specs.empty()) {
        LOG_ERROR("WorldMap 多层加载失败：map.layers 为空");
        return false;
    }

    // 1) layer id 唯一性校验
    std::vector<std::string> seenIds;
    for (const auto& spec : specs) {
        if (spec.id.empty()) {
            LOG_ERROR("WorldMap 多层加载失败：存在空 layer id");
            return false;
        }
        for (const auto& sid : seenIds) {
            if (sid == spec.id) {
                LOG_ERROR("WorldMap 多层加载失败：duplicate layer id -> " << spec.id);
                return false;
            }
        }
        seenIds.push_back(spec.id);
    }

    layers_.clear();
    int mapWidth = 0;
    int mapHeight = 0;

    for (size_t i = 0; i < specs.size(); ++i) {
        const MapLayerSpec& spec = specs[i];
        std::string content = ResourceManager::GetText(spec.file);
        if (content.empty()) {
            LOG_ERROR("WorldMap 多层加载失败：layer[" << spec.id << "] 文件缺失或为空 -> " << spec.file);
            return false;
        }

        int w = 0, h = 0;
        std::vector<int> flat;
        if (!ParseMapFile(content, w, h, flat)) {
            LOG_ERROR("WorldMap 多层加载失败：layer[" << spec.id << "] 矩阵无效/malformed -> " << spec.file);
            return false;
        }

        if (i == 0) {
            mapWidth = w;
            mapHeight = h;
        } else if (w != mapWidth || h != mapHeight) {
            LOG_ERROR("WorldMap 多层加载失败：layer[" << spec.id << "] 尺寸 "
                      << w << "x" << h << " 与首层 " << mapWidth << "x" << mapHeight
                      << " 不一致（不允许静默 resize）");
            return false;
        }

        layers_.emplace_back(w, h, TileLayer::kEmpty);
        for (int row = 0; row < h; ++row) {
            for (int col = 0; col < w; ++col) {
                layers_[i].SetTile(col, row, flat[static_cast<size_t>(row) * w + col]);
            }
        }
    }

    BuildTextures();
    RebuildMetadata();

    LOG_INFO("WorldMap 多层加载成功: " << specs.size() << " 层 (" << mapWidth << "x" << mapHeight << ")");
    return true;
}

bool WorldMap::LoadElevation(const std::string& elevationFile) {
    if (layers_.empty()) {
        LOG_ERROR("WorldMap::LoadElevation 失败：尚未加载任何地图层，无法匹配尺寸");
        return false;
    }
    const int expectW = Width();
    const int expectH = Height();

    std::string content = ResourceManager::GetText(elevationFile);
    if (content.empty()) {
        LOG_ERROR("WorldMap::LoadElevation 失败：高度文件缺失或为空 -> " << elevationFile);
        return false;
    }

    std::stringstream stream(content);
    std::string line;
    std::vector<uint8_t> values;
    int rows = 0;
    int width = -1;
    int lineNo = 0;
    while (std::getline(stream, line)) {
        ++lineNo;
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty()) continue;
        std::stringstream ls(line);
        int v;
        bool bad = false;
        std::vector<int> row;
        while (ls >> v) {
            if (v < 0 || v > 3) {
                LOG_ERROR("WorldMap::LoadElevation 失败：值越界 " << v
                          << " (合法范围 0..3) @line " << lineNo);
                return false;
            }
            row.push_back(v);
        }
        if (!(ls.eof() && ls.fail())) {
            // 含非整数 token
            LOG_ERROR("WorldMap::LoadElevation 失败：含有非整数令牌 @line " << lineNo);
            return false;
        }
        if (row.empty()) continue;
        if (width < 0) width = static_cast<int>(row.size());
        if (static_cast<int>(row.size()) != width) {
            LOG_ERROR("WorldMap::LoadElevation 失败：行宽不一致 @line " << lineNo
                      << " (本行 " << row.size() << "，首行 " << width << ")");
            return false;
        }
        for (int val : row) values.push_back(static_cast<uint8_t>(val));
        ++rows;
    }

    if (width <= 0 || rows <= 0) {
        LOG_ERROR("WorldMap::LoadElevation 失败：无法解析出有效矩阵 -> " << elevationFile);
        return false;
    }
    if (width != expectW || rows != expectH) {
        LOG_ERROR("WorldMap::LoadElevation 失败：尺寸不匹配 高度文件 " << width << "x" << rows
                  << "，地图 " << expectW << "x" << expectH
                  << "（不允许静默 resize，请修正高度文件或地图）");
        return false;
    }

    elevation_.swap(values);
    elevationW_ = width;
    elevationH_ = rows;
    hasElevation_ = true;
    LOG_INFO("WorldMap::LoadElevation 成功: " << width << "x" << rows
             << "，8位有符号仅用 0..3，已启用 HasElevation");
    return true;
}

uint8_t WorldMap::GetElevation(int col, int row) const {
    if (!hasElevation_) return 0;   // 无 elevation => 恒 baseline（向后兼容）
    if (col < 0 || row < 0 || col >= elevationW_ || row >= elevationH_) return 0;
    return elevation_[static_cast<size_t>(row) * elevationW_ + col];
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
    return QueryCell(col, row, kLayerMaskAll);
}

std::vector<std::pair<int, int>> WorldMap::QueryCell(int col, int row, LayerMask mask) const {
    std::vector<std::pair<int, int>> result;
    if (!layers_.empty()) {
        result.reserve(std::min<size_t>(layers_.size(), 64));
    }
    for (size_t i = 0; i < layers_.size(); ++i) {
        if ((mask & LayerMaskBit(i)) == 0) continue;   // 只查 mask 命中的层
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

void WorldMap::DumpCell(int col, int row) const {
    LOG_INFO("Cell (" << col << "," << row << ")");
    if (layers_.empty()) {
        LOG_INFO("  (no layers)");
        return;
    }
    for (size_t i = 0; i < layers_.size(); ++i) {
        const TileLayer& layer = layers_[i];
        int tile = layer.GetTile(col, row);
        if (tile == TileLayer::kEmpty) {
            LOG_INFO("  layer[" << i << "] tile=EMPTY");
            continue;
        }
        if (tile == TileLayer::kInvalid) {
            LOG_INFO("  layer[" << i << "] tile=INVALID(out of bounds)");
            continue;
        }
        bool solid = tileSet_.IsSolid(tile);
        bool overlay = tileSet_.IsOverlay(tile);
        std::string trigger = tileSet_.TriggerOf(tile);
        std::ostringstream line;
        line << "  layer[" << i << "] tile=" << tile
             << " solid=" << (solid ? "true" : "false")
             << " trigger=" << (trigger.empty() ? "none" : trigger)
             << " overlay=" << (overlay ? "true" : "false");
        LOG_INFO(line.str());
    }
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

    // Phase8: collect cliff walls into member for unified depth-sort in SceneManager.
    cliffQuads_.clear();

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
                // 统一投影：地面菱形中心根据该格高度在屏幕 y 上移（e!=0 时改变，baseline(0) 无影响）。
                int elev = hasElevation_ ? static_cast<int>(GetElevation(col, row)) : 0;
                SDL_Point c = proj.ProjectedFoot(wx, wy, elev, ElevationStep());
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

                // Phase8: collect exposed vertical cliff walls (neighbor lower than this cell)
                if (elev > 0) {
                    // dir -> (dc,dr) -> diamond edge (a,b)
                    struct Wall { int dc, dr, a, b; } walls[4] = {
                        { 0,-1, 1, 2 }, // N  (row-1) edge[1]-[2] screen-upper-right
                        { 1, 0, 2, 3 }, // E  (col+1) edge[2]-[3] screen-right
                        { 0, 1, 3, 0 }, // S  (row+1) edge[3]-[0] screen-lower-left
                        {-1, 0, 0, 1 }, // W  (col-1) edge[0]-[1] screen-upper-left
                    };
                    for (int wi = 0; wi < 4; ++wi) {
                        const Wall& w = walls[wi];
                        int ne = static_cast<int>(GetElevation(col + w.dc, row + w.dr));
                        if (ne >= elev) continue;            // not exposed (covered by equal/higher)
                        const SDL_Point& A = pts[w.a];
                        const SDL_Point& B = pts[w.b];
                        CliffQuad q;
                        q.ax = A.x; q.ay = A.y; q.bx = B.x; q.by = B.y;
                        q.drop = (elev - ne) * ElevationStep();
                        q.tex = tex;
                        q.sortKey = static_cast<float>(A.y + q.drop); // bottom y
                        cliffQuads_.push_back(q);
                    }
                }

            }
        }
    }

}