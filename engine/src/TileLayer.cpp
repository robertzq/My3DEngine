#include "Engine/TileLayer.h"
#include "Engine/TileSet.h"
#include "Engine/TileMap.h"   // TileSprite 完整定义（overlay 纹理/sortY）
#include "Engine/Texture.h"
#include "Engine/Log.h"
#include <map>

void TileLayer::Resize(int w, int h, int fill) {
    if (w < 0 || h < 0) {
        LOG_ERROR("TileLayer::Resize 非法尺寸 (" << w << "x" << h << ")");
        return;
    }
    width_ = w;
    height_ = h;
    data_.assign(static_cast<size_t>(w) * h, fill);
}

int TileLayer::GetTile(int col, int row) const {
    if (!InBounds(col, row)) return kInvalid;
    const int id = data_[static_cast<size_t>(row) * width_ + col];
    return id;
}

bool TileLayer::SetTile(int col, int row, int id) {
    if (!InBounds(col, row)) {
        LOG_ERROR("TileLayer::SetTile 越界 "
                  << "(" << col << "," << row << ") 越界于 "
                  << width_ << "x" << height_);
        return false;
    }
    data_[static_cast<size_t>(row) * width_ + col] = id;
    return true;
}

void TileLayer::RebuildMetadata(const TileSet& tileSet, int tileSize,
                                const std::map<int, Texture*>* textures) {
    colliders_.clear();
    triggerNames_.clear();
    triggerRects_.clear();
    overlays_.clear();

    int width = Width();
    int height = Height();
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            int id = GetTile(col, row);
            // -1 = Empty：本层该 cell 无 tile，不生成任何元数据。
            if (id == kEmpty || id == kInvalid) continue;

            SDL_Rect rect = {col * tileSize, row * tileSize, tileSize, tileSize};

            if (tileSet.IsSolid(id)) colliders_.push_back(rect);

            std::string trigger = tileSet.TriggerOf(id);
            if (!trigger.empty()) {
                int idx = -1;
                for (size_t i = 0; i < triggerNames_.size(); ++i) {
                    if (triggerNames_[i] == trigger) { idx = static_cast<int>(i); break; }
                }
                if (idx < 0) {
                    triggerNames_.push_back(trigger);
                    triggerRects_.emplace_back();
                    idx = static_cast<int>(triggerNames_.size() - 1);
                }
                triggerRects_[idx].push_back(rect);
            }

            if (tileSet.IsOverlay(id)) {
                // overlay 的 sortY 沿用“行底 y = rect.y + rect.h”（与 TileMap 一致的 Y-sort 语义）。
                TileSprite sp;
                sp.rect = rect;
                sp.sortY = rect.y + rect.h;
                sp.col = col;
                sp.row = row;
                sp.texture = nullptr;
                if (textures) {
                    auto tIt = textures->find(id);
                    if (tIt != textures->end()) sp.texture = tIt->second;
                }
                overlays_.push_back(sp);
            }
        }
    }
}

std::vector<SDL_Rect> TileLayer::TriggerRects(const std::string& name) const {
    for (size_t i = 0; i < triggerNames_.size(); ++i) {
        if (triggerNames_[i] == name) return triggerRects_[i];
    }
    return {};
}

bool TileLayer::FindTile(int id, int& outCol, int& outRow) const {
    int width = Width();
    int height = Height();
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            if (GetTile(col, row) == id) {
                outCol = col;
                outRow = row;
                return true;
            }
        }
    }
    return false;
}