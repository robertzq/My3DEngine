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

    std::printf("=== 完成：%d 项 / 失败 %d 项 ===\n", g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}