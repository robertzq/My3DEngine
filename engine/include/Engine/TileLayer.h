#pragma once
#include <vector>
#include <map>
#include <cstdint>
#include <string>
#include "SDL.h"

class TileSet;
struct TileSprite;
class Texture;

// ============================================================================
// TileLayer —— 地图系统优化（第二阶段）新增的单层 Tile 数据模型。
//
// 设计要点（对齐需求文档）：
//   1. 一层 = 一个独立矩阵 data[row][col] = tileId。
//   2. -1 表示 Empty：本层该 cell 没有 tile。数据按原始值保存，
//      绝不跨层继承（“看到下面的层”是渲染/查询结果，不是数据继承）。
//   3. 轻量、可拷贝、无堆分配的花哨结构；主要负责“存数据 + 按需生成碰撞/
//      叠加/触发元数据 + 遍历”，不承担渲染排序策略（排序由上层决定）。
//   4. 尺寸在构造/Resize 时固定，访问越界断言/安全返回 Empty。
// ============================================================================
class TileLayer {
public:
    static constexpr int kEmpty = -1;           // Empty tile id（空 cell）
    static constexpr int kInvalid = -2;         // 越界访问返回值

    TileLayer() = default;
    TileLayer(int w, int h, int fill = kEmpty) { Resize(w, h, fill); }

    // 尺寸
    int Width()  const { return width_; }
    int Height() const { return height_; }
    bool Empty()  const { return width_ == 0 || height_ == 0; }
    bool InBounds(int col, int row) const {
        return col >= 0 && row >= 0 && col < width_ && row < height_;
    }

    // 调整尺寸，新区域填充 fill（默认 Empty），保留重叠部分原数据。
    void Resize(int w, int h, int fill = kEmpty);

    // 原始数据访问（row-major：data_[row][col]）
    // GetTile：越界返回 kInvalid；空 cell 返回 kEmpty。
    int  GetTile(int col, int row) const;
    // SetTile：越界直接忽略（记录调试日志），否则写入。
    bool SetTile(int col, int row, int id);

    // 按 tileSet 重建本层元数据（colliders / overlays / triggers）。
    // tileSize 由上层（WorldMap 持有 tileSet）传入，用于把逻辑格换算成像素矩形。
    // textures 是 tileId -> Texture* 的查找表（由上层持有），用于给 overlay 填充纹理。
    // 每层独立生成；上层 WorldMap 负责把多层的 ground/overlay/trigger 聚合给渲染与物理。
    void RebuildMetadata(const TileSet& tileSet, int tileSize,
                         const std::map<int, Texture*>* textures = nullptr);

    // 元数据访问（重建后有效）：
    const std::vector<SDL_Rect>& Colliders()  const { return colliders_; }
    const std::vector<TileSprite>& Overlays() const { return overlays_; }
    // 指定名字的触发区矩形列表（无则返回空）
    std::vector<SDL_Rect> TriggerRects(const std::string& name) const;

    // 该层内某个 tileId 首次出现位置（供工具/测试），不存在返回 false。
    bool FindTile(int id, int& outCol, int& outRow) const;

    // 原始数据只读句柄（调试/序列化用），不建议业务直接改。
    const std::vector<int>& Raw() const { return data_; }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<int> data_;
    std::vector<SDL_Rect> colliders_;
    std::vector<TileSprite> overlays_;
    // 触发区：name -> rects
    std::vector<std::string> triggerNames_;
    std::vector<std::vector<SDL_Rect>> triggerRects_;
};