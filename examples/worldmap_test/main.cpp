// WorldMap 多层空间数据系统 - 最小单元测试（阶段3）。
// 聚焦数据 + 查询语义（不依赖渲染/资源管线）：
//   1) TileLayer: GetTile/SetTile/-1 Empty/越界
//   2) WorldMap 多层: QueryCell 返回完整非空层栈（不 flatten）
//   3) QueryCell 层序稳定；高层独立可见（不继承低层）
//   4) SetTile 层级路由（SetGroundTile / SetTile(layer) / 老 SetTile->ground）
//   5) 多层尺寸/层数
// 运行：main 返回 0 全部通过，>0 存在失败项。

#define SDL_MAIN_HANDLED   // 本测试是纯逻辑 console，避免 SDL2main 劫持 main
#include "Engine/WorldMap.h"
#include "Engine/TileLayer.h"
#include "Engine/TileSet.h"
#include "Engine/Projection.h"
#include <cstdio>
#include <vector>

static int g_checks = 0;
static int g_fail = 0;
#define CHECK(cond, msg) do { ++g_checks; if (!(cond)) { std::printf("[FAIL] %s\n", msg); ++g_fail; } else { std::printf("[OK]   %s\n", msg); } } while (0)

int main() {
    std::printf("=== WorldMap 多层数据语义测试 ===\n");

    // --- Test A: TileLayer 基础 ---
    {
        TileLayer layer(3, 2, TileLayer::kEmpty);
        CHECK(layer.Width() == 3 && layer.Height() == 2, "A1 layer 尺寸");
        CHECK(layer.GetTile(1, 1) == TileLayer::kEmpty, "A2 新建层全空=-1");
        CHECK(layer.SetTile(1, 1, 5), "A3 SetTile 成功");
        CHECK(layer.GetTile(1, 1) == 5, "A4 GetTile 读回");
        CHECK(layer.GetTile(0, 0) == TileLayer::kEmpty, "A5 未设置处仍 -1");
        CHECK(layer.GetTile(9, 9) == TileLayer::kInvalid, "A6 越界返回 Invalid");
        CHECK(layer.GetTile(1, 9) == TileLayer::kInvalid, "A7 行越界");
    }

    // --- Test B: WorldMap 多层 + 尺寸 ---
    {
        WorldMap wm;
        wm.AddLayer(3, 3, TileLayer::kEmpty);
        wm.AddLayer(3, 3, TileLayer::kEmpty);
        CHECK(wm.LayerCount() == 2, "B1 层数=2");
        CHECK(wm.Width() == 3 && wm.Height() == 3, "B2 尺寸 3x3");
        CHECK(wm.TileSize() == 32, "B3 默认 tileSize=32");
        CHECK(wm.Empty() == false, "B4 非空");
    }

    // --- Test C: QueryCell 完整非空层栈（不 flatten）---
    {
        WorldMap wm;
        wm.AddLayer(3, 3, TileLayer::kEmpty);   // 0: ground
        wm.AddLayer(3, 3, TileLayer::kEmpty);   // 1: terrain
        wm.AddLayer(3, 3, TileLayer::kEmpty);   // 2: water
        wm.SetGroundTile(1, 1, 0);
        wm.SetTile(1, 1, 1, 2);
        wm.SetTile(2, 1, 1, 5);
        auto stack = wm.QueryCell(1, 1);
        CHECK(stack.size() == 3, "C1 完整层栈=3 (不flatten)");
        if (stack.size() == 3) {
            CHECK(stack[0].first == 0 && stack[0].second == 0, "C2 栈[0] Ground/0");
            CHECK(stack[1].first == 1 && stack[1].second == 2, "C3 栈[1] Terrain/2");
            CHECK(stack[2].first == 2 && stack[2].second == 5, "C4 栈[2] Water/5");
        }
        CHECK(wm.QueryCell(0, 0).empty(), "C5 空 cell -> 空栈");
        CHECK(stack.size() == 3, "C6 未 flatten 成单 tile");
    }

    // --- Test D: 层序稳定 + 不跨层继承 ---
    {
        WorldMap wm;
        wm.AddLayer(2, 2, TileLayer::kEmpty);
        wm.AddLayer(2, 2, TileLayer::kEmpty);
        wm.SetTile(1, 0, 0, 3);   // 仅高层有值，低层空
        auto st = wm.QueryCell(0, 0);
        CHECK(st.size() == 1 && st[0].first == 1 && st[0].second == 3,
              "D1 高层独立可见(不继承低层 -1)");
        // 层序：stable 且按 layer index 递增
        wm.SetGroundTile(1, 1, 7);
        wm.SetTile(1, 1, 1, 9);
        auto st2 = wm.QueryCell(1, 1);
        CHECK(st2.size() == 2 && st2[0].first == 0 && st2[1].first == 1,
              "D2 层序稳定(Ground先, Water后)");
    }

    // --- Test E: SetTile 层级路由 ---
    {
        WorldMap wm;
        wm.AddLayer(2, 2, TileLayer::kEmpty);
        wm.AddLayer(2, 2, TileLayer::kEmpty);
        wm.SetGroundTile(0, 0, 7);
        wm.SetTile(1, 0, 0, 8);
        CHECK(wm.GetGroundTile(0, 0) == 7, "E1 SetGroundTile->ground");
        CHECK(wm.GetTile(1, 0, 0) == 8, "E2 SetTile(layer=1) 精确层");
        CHECK(wm.QueryCell(0, 0).size() == 2, "E3 两层都有值");
        wm.SetTile(1, 1, 9);    // 老 SetTile(x,y,id) -> default/ground
        CHECK(wm.GetGroundTile(1, 1) == 9, "E4 老 SetTile 走 ground 层");
        // 精确层查询与栈一致
        CHECK(wm.GetTile(0, 1, 1) == 9, "E5 ground层(1,1)=9");
        CHECK(wm.GetTile(1, 1, 1) == TileLayer::kEmpty, "E6 terrain层(1,1)仍空");
    }

    // --- Test F: QueryCell(x,y,mask) LayerMask 掩码查询（不写死四层）---
    {
        WorldMap wm;
        wm.AddLayer(3, 3, TileLayer::kEmpty);   // 0: ground
        wm.AddLayer(3, 3, TileLayer::kEmpty);   // 1: terrain
        wm.AddLayer(3, 3, TileLayer::kEmpty);   // 2: water
        wm.AddLayer(3, 3, TileLayer::kEmpty);   // 3: decoration
        wm.SetGroundTile(1, 1, 1);              // ground         = 1
        wm.SetTile(1, 1, 1, 2);                 // terrain        = 2
        wm.SetTile(2, 1, 1, 3);                 // water          = 3
        wm.SetTile(3, 1, 1, 5);                 // decoration     = 5

        // Case C: mask = Ground | Water -> 只返回这两层
        auto mask = WorldMap::LayerMaskBit(0) | WorldMap::LayerMaskBit(2);
        auto q = wm.QueryCell(1, 1, mask);
        CHECK(q.size() == 2, "F1 mask(Ground|Water) 只返回2层");
        if (q.size() == 2) {
            CHECK(q[0].first == 0 && q[0].second == 1, "F2 mask 命中的 Ground/1");
            CHECK(q[1].first == 2 && q[1].second == 3, "F3 mask 命中的 Water/3");
        }
        CHECK(wm.QueryCell(1, 1).size() == 4, "F4 无 mask(全层)返回完整4层栈");
        CHECK(wm.QueryCell(1, 1, WorldMap::LayerMaskBit(1)).size() == 1,
              "F5 单层 mask terrain 只返回1项");
        CHECK(wm.QueryCell(1, 1, WorldMap::LayerMaskBit(63) | WorldMap::LayerMaskBit(0))
                    .size() == 1,
              "F6 超层数 mask(bit63)被忽略, 仅返回 ground");
    }

    // --- Test G: 多层 solid / trigger 跨层收集（P6）---
    {
        // TileSet：id=1 solid, id=2 trigger("door"), id=5 overlay（无纹理，仅验证收集）
        TileSet ts; ts.tileSize = 16;
        ts.tiles[1] = TileDef{ "", true, false, "" };
        ts.tiles[2] = TileDef{ "", false, false, "door" };
        ts.tiles[5] = TileDef{ "", false, true, "" };

        WorldMap wm;
        wm.AddLayer(2, 2, TileLayer::kEmpty);   // 0: ground
        wm.AddLayer(2, 2, TileLayer::kEmpty);   // 1: terrain
        // ground 层 (0,0)=solid; terrain 层 (1,0)=solid; ground 层 (0,1)=trigger; terrain 层 (1,1)=overlay
        wm.SetGroundTile(0, 0, 1);
        wm.SetGroundTile(0, 1, 2);
        wm.SetTile(1, 1, 0, 1);
        wm.SetTile(1, 1, 1, 5);
        wm.SetTileSet(ts);   // 供 RebuildMetadata 使用（无纹理时 overlay 仍收集）
        wm.RebuildMetadata();

        CHECK(wm.Colliders().size() == 2, "G1 两层 solid 都收集(共2个)");
        CHECK(wm.TriggerRects("door").size() == 1, "G2 ground 层 trigger 收集");
        CHECK(wm.Overlays().size() == 1, "G3 terrain 层 overlay 收集");

        // 位置校验：door trigger 不在 solid 集合里（不同语义孤立收集）
        auto dr = wm.TriggerRects("door");
        CHECK(!dr.empty() && dr[0].x == 0 && dr[0].y == 16, "G4 trigger rect 位置正确");
        // solid 之一在 (1,0) terrain 层
        bool hasTerrainSolid = false;
        for (auto& r : wm.Colliders()) {
            if (r.x == 16 && r.y == 0) hasTerrainSolid = true;
        }
        CHECK(hasTerrainSolid, "G5 terrain 层 solid 可被物理消费");
    }

// --- Test H: runtime mutation 不产生 stale cache（P7）---
    {
        TileSet ts; ts.tileSize = 16;
        ts.tiles[1] = TileDef{ "", true,  false, "" };   // solid
        ts.tiles[2] = TileDef{ "", false, false, "door" }; // trigger
        ts.tiles[5] = TileDef{ "", false, true,  "" };   // overlay

        WorldMap wm;
        wm.AddLayer(2, 2, TileLayer::kEmpty);
        wm.SetTileSet(ts);

        // Empty -> solid：写后重建，collider 出现
        wm.SetTile(0, 1, 1, 1);
        wm.RebuildMetadata();
        CHECK(wm.Colliders().size() == 1, "H1 empty->solid 后 collider 出现");

        // solid -> Empty：cell 置空（删掉），重建后 collider 消失（不 stale）
        wm.SetTile(0, 1, 1, TileLayer::kEmpty);
        wm.RebuildMetadata();
        CHECK(wm.Colliders().empty(), "H2 solid->empty 后 collider 清除(不 stale)");

        // Empty -> trigger：重建后 trigger 出现
        wm.SetTile(0, 0, 0, 2);
        wm.RebuildMetadata();
        CHECK(wm.TriggerRects("door").size() == 1, "H3 empty->trigger 后 trigger 出现");
        CHECK(wm.Colliders().empty(), "H4 trigger tile 非 solid, collider 仍空");

        // overlay 变更：写 overlay tile 后 overlay 出现；置空后清除
        wm.SetTile(0, 1, 0, 5);
        wm.RebuildMetadata();
        CHECK(wm.Overlays().size() == 1, "H5 empty->overlay 后 overlay 出现");
        wm.SetTile(0, 1, 0, TileLayer::kEmpty);
        wm.RebuildMetadata();
        CHECK(wm.Overlays().empty(), "H6 overlay->empty 后 overlay 清除(不 stale)");
    }

    // --- Test I: DumpCell 诊断输出（#20, 仅验证能编译+不改变状态）---
    {
        TileSet ts; ts.tileSize = 16;
        ts.tiles[1] = TileDef{ "", true,  false, "" };     // solid
        ts.tiles[2] = TileDef{ "", false, false, "door" }; // trigger
        ts.tiles[5] = TileDef{ "", false, true,  "" };     // overlay
        WorldMap wm;
        wm.AddLayer(2, 2, TileLayer::kEmpty);
        wm.AddLayer(2, 2, TileLayer::kEmpty);
        wm.SetTileSet(ts);
        wm.SetGroundTile(1, 1, 1);
        wm.SetTile(1, 1, 1, 5);   // (1,1) 两语义层都可 dump
        wm.RebuildMetadata();
        wm.DumpCell(1, 1);        // 打印 ground solid + terrain overlay
        wm.DumpCell(0, 0);        // 打印各层 EMPTY
        CHECK(wm.GetGroundTile(1, 1) == 1 && wm.GetTile(1, 1, 1) == 5,
              "I1 DumpCell 不改变数据状态");
    }

    // --- Test J: Projection::ProjectedFoot 统一投影 (elevation-aware) ---
    {
        Projection iso;
        iso.mode = ProjectionMode::Iso;
        iso.grid = {64, 32};
        iso.view.scale = 2.6f;
        iso.view.offsetX = 100.0f;
        iso.view.offsetY = 200.0f;
        const int step = 42;

        SDL_Point base = iso.WorldToScreen(928.0f, 560.0f);
        SDL_Point p0 = iso.ProjectedFoot(928.0f, 560.0f, 0, step);
        CHECK(p0.x == base.x && p0.y == base.y, "J1 elevation=0 与 WorldToScreen 一致");

        SDL_Point p1 = iso.ProjectedFoot(928.0f, 560.0f, 1, step);
        CHECK(p1.x == p0.x && p1.y == p0.y - step, "J2 elevation=1 只改屏幕 y (-一级)");

        SDL_Point p2 = iso.ProjectedFoot(928.0f, 560.0f, 2, step);
        CHECK(p2.y == p0.y - 2 * step && p2.x == p0.x, "J3 elevation=2 屏幕 y -二级, x 不变");

        Projection ortho;
        ortho.mode = ProjectionMode::Ortho;
        SDL_Point o1 = ortho.ProjectedFoot(100.0f, 80.0f, 3, step);
        CHECK(o1.x == 100 && o1.y == 80 - 3 * step, "J4 ortho 模式统一投影同样生效");
    }

    // --- Test K: Phase9 elevation movement gate ---
    {
        WorldMap wm;
        wm.AddLayer(3, 3, TileLayer::kEmpty);   // default tileSize=32
        CHECK(!wm.HasElevation(), "K1 初始无 elevation");
        CHECK(wm.SetElevationCell(0, 0, 1), "K2 注入 elev(0,0)=1");
        CHECK(wm.SetElevationCell(1, 0, 1), "K3 注入 elev(1,0)=1");
        CHECK(wm.SetElevationCell(1, 1, 0), "K4 注入 elev(1,1)=0");
        CHECK(wm.HasElevation(), "K5 已启用 elevation");
        CHECK(wm.CellCol(40.0f) == 1 && wm.CellRow(20.0f) == 0, "K6 CellCol/Row");
        CHECK(wm.IsElevationMoveAllowed(10, 10, 40, 10), "K7 同高度 (0,0)->(1,0) allowed");
        CHECK(!wm.IsElevationMoveAllowed(10, 10, 40, 42), "K8 elev1 -> elev0 blocked");
        CHECK(wm.IsElevationMoveAllowed(10, 10, 15, 12), "K9 同单元 allowed");
        CHECK(!wm.SetElevationCell(3, 0, 2), "K10 越界拒绝");
        WorldMap plain;
        plain.AddLayer(3, 3, TileLayer::kEmpty);
        CHECK(plain.IsElevationMoveAllowed(0, 0, 200, 200), "K11 无 elevation 恒 allowed");
    }

    std::printf("=== 完成：%d 项 / 失败 %d 项 ===\n", g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}