# Elevation Regression Baseline — Phase 0

Recorded before any elevation changes (feature/elevation-map).

## Repos / build
- Engine source (changes): `F:\My3DEngine` branch `feature/elevation-map`, VS2022 x64 generator.
- Game host (smoke) : `F:\memIslandCode\myMemIsland` master, links engine via `add_subdirectory(F:/My3DEngine)`.
- Engine unit tests    : `WorldMapTest` (`MYENGINE_BUILD_TESTS=ON`), run with SDL dlls copied beside exe.

## Engine unit tests (WorldMapTest)
- Build: PASS (engine.lib + WorldMapTest.exe, Debug/x64)
- Run: ALL CHECKs pass, 3 blocks / 0 failures, exit 0
- Covers: TileLayer GetTile/SetTile/-1/out-of-range, WorldMap size/QueryCell stack/
  flatten, layer priority/inheritance, SetTile routing, LayerMask query, DumpCell.

## Smoke (island_gen)
- `MyMemIsland.exe --scene island_gen --shot 10` → exit 0
- Screenshot saved: `elevation_baseline_island.bmp` (1875 KB, 65x?), also png
  `screenshot_preview.png`.

## Data snapshot (island_gen, from config.json)
- map dims        : 44 x 44
- layers          : 4 (ground/terrain/water/decoration)
- entities        : 31 behavior objects, 30 at[] coords
- projection      : iso
- projection_scale: 2.6
- view            : IslandView
- spawn           : default
- tileset         : layered (ground=0 grass, water=1 solid, deep_grass=2 overlay,
  sand=3, path=4, plaza=5 trigger)

## Baseline anchors (must stay identical at elevation=0)
- WorldToScreen(wx,wy) == ProjectedFootPoint(wx,wy, elev=0)
- SortKey == transform.y + transform.h + sortYOffset for all entities
- overlay sortY == rect.y + rect.h
- ground diamond centered at projected cell-center
- camera follows player foot (ortho world pos)