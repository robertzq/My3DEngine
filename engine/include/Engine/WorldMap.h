#pragma once
#include <SDL.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Engine/TileSet.h"
#include "Engine/TileLayer.h"
#include "Engine/Projection.h"

class Texture;
struct TileSprite;

// ============================================================================
// WorldMap —— 地图系统优化（第二阶段）的多层地图容器。
//
// 设计要点（对齐需求文档，additive migration）：
//   1. WorldMap 是一组有序 TileLayer。Layer 由 LayerId/下标标识，
//      不写死“只有四层”——是多层的，由 config 的 layers[] 决定。
//   2. -1 = Empty 保存在各层原始数据里，绝不跨层继承。
//      QueryCell(x,y) 返回该 cell 所有层的非空栈（稳定顺序：层序），
//      供 Behavior 按 LayerMask 做空间查询，而不是 flatten 成“一格一最终 tile”。
//   3. 兼容：旧版 “map”: “maps/island.map” 字符串经 LoadLegacyMap 包装为
//      一个默认 ground 层，Colliders/Overlays/TriggerRects/WidthPx/HeightPx/
//      DrawGround/DrawGroundIso 语义与旧 TileMap 完全一致，调用方零改动。
//   4. 顶层（Phase3 后）提供 layers[] 容器 + validation。
// ============================================================================
class WorldMap {
public:
    // —— 生命周期 / 加载 ——
    // 旧单矩阵地图：作为默认 ground 层加载（高度兼容，保持旧行为）。
    // 返回 false 表示加载失败（资源为空/无有效数据）。
    bool LoadLegacyMap(const std::string& mapResourceId, const TileSet& tileSet);

    // —— 多层 API（Phase3 起走这里；Phase2 提供基础能力）——
    // 追加一个空层。返回 layerId（从 0 递增）。
    int AddLayer(int width, int height, int fill = TileLayer::kEmpty);
    // 按下标取层；越界返回 nullptr（non-owning）。
    TileLayer* LayerAt(size_t index);
    const TileLayer* LayerAt(size_t index) const;
    // 层数
    size_t LayerCount() const { return layers_.size(); }

    // QueryCell：返回该 cell 所有非空层的栈（层序/元素对），供空间查询。
    // 返回 vector<pair<layerIndex, tileId>>；空 cell 返回空 vector。
    std::vector<std::pair<int, int>> QueryCell(int col, int row) const;

    // —— 逐层 Tile 读写（越界安全）——
    int  GetTile(int layerIndex, int col, int row) const;
    bool SetTile(int layerIndex, int col, int row, int id);
    // 默认层（第 0 层）的快捷 Set/Get，兼容旧 SetTile(col,row,id) 语义。
    int  GetGroundTile(int col, int row) const;
    bool SetGroundTile(int col, int row, int id);

    // —— 重建全部层元数据（改任意层后调用）——
    void RebuildMetadata();

    // —— 渲染 ——
    // ground pass：画所有“非 overlay”格（多层合并，按层序先下后上）。
    void DrawGround(const SDL_Rect& camera) const;
    void DrawGroundIso(const Projection& proj, const SDL_Rect& camera) const;

    // —— 空间查询（兼容旧 TileMap 接口，调用方零改动）——
    const std::vector<SDL_Rect>& Colliders() const { return colliders_; }
    std::vector<SDL_Rect> TriggerRects(const std::string& name) const;
    const std::vector<TileSprite>& Overlays() const { return overlays_; }
    std::vector<SDL_Rect> TilesWithId(int id) const;

    void SetTile(int col, int row, int id) { SetGroundTile(col, row, id); }

    // —— 尺寸 / 元信息 ——
    bool Empty() const { return layers_.empty() || layers_[0].Empty(); }
    int Width() const { return layers_.empty() ? 0 : layers_[0].Width(); }
    int Height() const { return layers_.empty() ? 0 : layers_[0].Height(); }
    int TileSize() const { return tileSize_; }
    int WidthPx() const { return Width() * tileSize_; }
    int HeightPx() const { return Height() * tileSize_; }
    const TileSet& TileSetRef() const { return tileSet_; }

private:
    // 把 tileId -> Texture* 纹理表重建自 tileSet（仅一次，供 overlay 填充）。
    void BuildTextures();

    std::vector<TileLayer> layers_;
    TileSet tileSet_;
    int tileSize_ = 32;
    std::map<int, Texture*> textures_;

    // 聚合元数据（跨全部层合并）
    std::vector<SDL_Rect> colliders_;
    std::vector<TileSprite> overlays_;
    std::vector<std::string> triggerNames_;
    std::vector<std::vector<SDL_Rect>> triggerRects_;
};