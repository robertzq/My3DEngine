# 地图数据格式（Map Format）

本文档描述引擎 `WorldMap` / `TileLayer` 多层空间数据系统的地图文件格式与加载规则。

## 1. 概览

地图由若干**图层（Layer）**组成，每个图层是一张独立的整数矩阵：

- 每个格子的值是一个 **tile id**（对应 `tileset` 里的定义），或特殊值 `-1`（空 / Empty）。
- `-1` 表示该层此格为空且**空不占位**：渲染时它表示"看见下一层"，但它**绝不跨层继承**数据（可见性是渲染结果，不是数据继承）。
- 图层有**稳定层序**：第 0 层通常为 Ground（地面），更高层为 Terrain / Water / Decoration 等。同一格可同时拥有多层的语义。

## 2. 配置项（config.json）

场景里 `map` 支持两种形态，**内部统一降级/升级为 WorldMap**：

### 2.1 旧格式（单地面层，向后兼容）

```json
"map": "maps/island.map"
```

等价于只有一层 Ground 的 WorldMap：

```
"map": {
  "layers": [ { "id": "ground", "file": "maps/island.map" } ]
}
```

旧格式仍完全合法，引擎内部映射为默认 Ground 层，island 等既有场景无需改动。

### 2.2 多层格式

```json
"map": {
  "layers": [
    { "id": "ground",     "file": "maps/layered_ground.map" },
    { "id": "terrain",    "file": "maps/layered_terrain.map" },
    { "id": "water",      "file": "maps/layered_water.map" },
    { "id": "decoration", "file": "maps/layered_decoration.map" }
  ]
}
```

- `id`：图层唯一标识，非空字符串，同一场景内不可重复。
- `file`：地图文件路径，相对 `assets/` 或 `maps/`。
- `layers` 数组顺序即层序（下标小的在底层）。

## 3. 地图文件格式（.map）

每个图层一个文本文件，是一个**矩形整数矩阵**：

```
0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0
0 0 4 4 4 4 4 0
0 0 4 4 4 4 4 0
-1 -1 -1 -1 -1 -1 -1 -1
-1 -1 -1 -1 -1 -1 -1 -1
```

规则：

- 每行由空格分隔的整数组成；也支持紧凑数字排布（无空格需靠解析器兼容）。
- 合法值：`>= 0` 的 tile id，或 `-1`（空）。`-2` 为内部无效哨兵值。
- 所有行宽度必须一致（矩形），否则视为 **malformed**。

### 4. 加载校验（LoadLayered）

`WorldMap::LoadLayered(specs, tileSet)` 按序执行以下校验，任一失败则**整体加载失败**（`map` 置空，不静默降级）：

| 校验 | 规则 | 失败行为 |
| --- | --- | --- |
| id 唯一性 | 同场景 layer id 不可重复 | `LOG_ERROR`: duplicate layer id |
| 空 id | id 必须非空 | `LOG_ERROR`: empty layer id |
| 文件缺失 | 文件存在且非空 | `LOG_ERROR`: file missing or empty |
| 矩阵格式 | 可解析为矩形整数矩阵 | `LOG_ERROR`: matrix malformed |
| 尺寸一致 | **首层定尺寸**，后续层宽度高度必须一致 | `LOG_ERROR`: dimension mismatch（不允许静默 resize） |

## 5. 查询与修改 API

### 5.1 查询一格的完整层栈（不 flatten）

```cpp
// 返回该格所有非空层的 {layerIndex, tileId} 栈，层序稳定（底层在前）。
std::vector<std::pair<int,int>> WorldMap::QueryCell(int col, int row);
```

- 同一格可同时具每层语义（如 Ground=0, Water=1），返回完整傅，**不退化为"一格一个最终 tile"**。
- `-1`（空）层不进入结果。

### 5.2 按 LayerMask 查询

```cpp
using LayerMask = uint64_t;            // bit0 = layer0, ..., 最多 64 层
std::vector<std::pair<int,int>> WorldMap::QueryCell(int col, int row, LayerMask mask);
```

只返回 `mask` 命中的层。不写死四层，按位选择语义层。

### 5.3 精确读写

```cpp
int  WorldMap::GetTile(int layer, int col, int row) const;   // 指定层的值，越界/空返回 -1
bool WorldMap::SetTile(int layer, int col, int row, int id); // 指定层写入
int  WorldMap::GetGroundTile(int col, int row) const;        // ground 层读
bool WorldMap::SetGroundTile(int col, int row, int id);      // ground 层写
bool WorldMap::SetTile(int col, int row, int id);            // 兼容：仅写 ground 层
```

### 5.4 诊断

```cpp
void WorldMap::DumpCell(int col, int row);   // 输出该格每层的 tile/solid/trigger/overlay 到日志
```

## 6. 渲染与物理语义

- **Ground pass**：所有**非 overlay** 的 tile 由 GroundPass 经 etc/等距投影绘制（`DrawGround` / `DrawGroundIso`），跨层聚合但只画非 overlay。
- **Depth-sorted**：**overlay** tile（`tileset` 里标记 `overlay: true`）与 Entity 一起进入 `DrawWorld`，按 `stable_sort`（Y 排序）统一排：
  - 人物在 decoration 之后 → decoration 覆盖人物；
  - 人物走到 decoration 前面 → 人物覆盖 decoration。
- **Solid**：`tileset` 里标记 `solid: true` 的 tile，从各层的 collider 收集，Physics 消费。
- **Trigger**：`tileset` 里标记 `trigger` 的 tile，跨层收集为触发区矩形，供场景过渡消费。

## 7. 示例 tileset（layered）

```json
"layered": {
  "tile_size": 32,
  "tiles": {
    "0": { "texture": "tiles_iso/grass.png" },
    "1": { "texture": "tiles_iso/water.png", "solid": true },
    "2": { "texture": "tiles_iso/deep_grass.png", "overlay": true },
    "3": { "texture": "tiles_iso/sand.png" },
    "4": { "texture": "tiles_iso/path.png" }
  }
}
```

## 8. 完整 Demo（layered_demo）

内置 `layered_demo` 场景演示 4 层（ground / terrain / water / decoration）+ 玩家与 NPC，可通过命令行直接进入：

```bash
MyMemIsland.exe --no-menu --shot 200 --scene layered_demo
```

运行时日志应显示：

```
[INFO] WorldMap 多层加载成功: 4 层 (8x8)
[INFO] SceneManager: 进入场景 layered_demo (spawn=default)
```