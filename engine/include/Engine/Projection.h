#pragma once
#include <SDL.h>
#include <cmath>
#include <cstdint>

// ============================================================================
// 引擎投影抽象层（header-only）
// 职责：把「世界逻辑坐标 -> 屏幕像素坐标」的映射收口到一处。
// 被 TileMap / Entity / SceneManager 共用，正交与斜等距可随时切换。
//
// 设计契约：
//   1. 世界坐标 (wx, wy) 始终是「逻辑网格坐标」（像素量级，与地图数据一致）。
//      游戏逻辑（碰撞/交互/移动/寻路）只感知世界坐标，不感知投影；
//      投影只发生在渲染侧。这样改造不影响已稳定的玩法代码。
//   2. 海拔 z：物体的立绘高度，渲染时从脚底向上抬升（物体“立在地上”）。
//   3. 排序（画家算法）仍按逻辑 wy 比较——等距下 wy 即屏幕纵向深度，天然正确，
//      因此 SceneManager 现有 stable_sort 逻辑无需改动。
//   4. 本层是纯函数集合，不持有全局可变状态。
//
// 等距 2:1（仙剑98 类）数学：
//   地面绕垂直轴旋转 45°，再沿垂直方向压缩一半（保持 2:1 菱形）。
//   世界点 (wx, wy) 到等距屏幕：
//     sx = (wx - wy) / 2
//     sy = (wx + wy) / 4
//   这一步把 正方形世界 映射成菱形（宽 2 倍于高）。
//   然后通过 view 缩放 + 偏移，把整个菱形世界映射到屏幕视口。
// ============================================================================

enum class ProjectionMode {
    Ortho,  // 正交平铺（原引擎默认）
    Iso,    // 斜等距 2:1（菱形，仙剑98 类俯视）
};

// 等距视图参数：把「世界菱形」映射进屏幕视口。
struct IsoView {
    float scale = 1.0f;   // 世界->屏幕的等距缩放（默认1，可按地图尺寸校准）
    float offsetX = 0.0f; // 屏幕偏移
    float offsetY = 0.0f;
};

// 菱形地面单元的几何参数（2:1：水平全宽 W = 垂直全高 H × 2）。
struct IsoGrid {
    int W = 64;   // 菱形水平全宽（屏幕上呈现的格宽）
    int H = 32;   // 菱形垂直全高

    int halfW() const { return W / 2; }
    int halfH() const { return H / 2; }
};

class Projection {
public:
    ProjectionMode mode = ProjectionMode::Ortho;
    IsoGrid grid;      // 等距菱形几何（正交模式下仅用于对齐参考）
    IsoView view;      // 等距视图映射（缩放/偏移）

    // —— 纯净的等距 2:1 映射（不缩放、不偏移，供计算菱形顶点用）——
    SDL_Point IsoPoint(float wx, float wy) const {
        // 用菱形几何 W/H 校准单位：世界每差 W 沿主轴一个菱形。
        // 但世界坐标直接是像素（地图 tile），此处以格宽=grid.W 为一格。
        // 标准：sx = (wx - wy) / 2, sy = (wx + wy) / 4。
        float sx = (wx - wy) * 0.5f;
        float sy = (wx + wy) * 0.25f;
        return { static_cast<int>(std::floor(sx)), static_cast<int>(std::floor(sy)) };
    }

    // 世界逻辑坐标 -> 屏幕坐标（含 view 缩放/偏移）。
    SDL_Point WorldToScreen(float wx, float wy) const {
        if (mode == ProjectionMode::Ortho) {
            return { static_cast<int>(wx), static_cast<int>(wy) };
        }
        SDL_Point iso = IsoPoint(wx, wy);
        float sx = iso.x * view.scale + view.offsetX;
        float sy = iso.y * view.scale + view.offsetY;
        return { static_cast<int>(std::floor(sx)), static_cast<int>(std::floor(sy)) };
    }

    // 带海拔 z：对象脚底投影在 (wx, wy, 0)，贴图向上延伸到头顶。
    // 返回「脚下落在屏幕的点」。
    SDL_Point Foot(float wx, float wy, float z) const {
        if (mode == ProjectionMode::Ortho) {
            SDL_Point p{ static_cast<int>(wx), static_cast<int>(wy) };
            p.y -= static_cast<int>(z);
            return p;
        }
        SDL_Point base = WorldToScreen(wx, wy);
        base.y -= static_cast<int>(z);
        return base;
    }


    // elevation-aware 统一投影：把「世界脚点 (wx,wy) + 地形高度差」投影成屏幕脚点。
    // elevationStepPx 是屏幕像素单位，仅作用于屏幕 y（不改变世界/碰撞坐标）。
    // 返回「地面脚底屏幕点」，调用方再往上减贴图高度即可（勿再减 elevation）。
    SDL_Point ProjectedFoot(float wx, float wy, int elevLevel, int elevStepPx) const {
        SDL_Point p = WorldToScreen(wx, wy);
        p.y -= elevLevel * elevStepPx;
        return p;
    }
    // —— 控制方向逆投影 ——
    // 把「屏幕方向意图（按键单位向量 dsx,dsy，例如按上=(-1)? 上=0,-1；右=1,0；下=0,1；左=-1,0）」
    // 逆投影为「世界单位移动向量」。等距下投影会把世界方向旋转 45°，
    // 为避免「按上却往屏幕右上走」的斜移感，按键应表示屏幕方向，再反解世界方向：
    //   dwx = dsx + 2*dsy
    //   dwy = 2*dsy - dsx
    // 返回值是单位向量（世界坐标移动方向），正交模式下原样返回。
    SDL_FPoint ScreenDirToWorldDir(float dsx, float dsy) const {
        if (mode != ProjectionMode::Iso) return {dsx, dsy};
        float wx = dsx + 2.0f * dsy;
        float wy = 2.0f * dsy - dsx;
        float len = std::sqrt(wx * wx + wy * wy);
        if (len < 1e-6f) return {0.0f, 0.0f};
        return {wx / len, wy / len};
    }

    // 生成一个菱形四顶点（屏幕坐标，中心在 cx,cy）。
    // 用于地面/瓦片以菱形铺装；菱形外 alpha 交给贴图/着色裁剪。
    // 注意：菱形几何必须与 WorldToScreen 的 view.scale 一致，否则与相邻格间距不匹配会露缝。
    void Diamond(int cx, int cy, SDL_Point out[4]) const {
        float s = (mode == ProjectionMode::Iso) ? view.scale : 1.0f;
        int hw = static_cast<int>(grid.halfW() * s);
        int hh = static_cast<int>(grid.halfH() * s);
        out[0] = { cx - hw, cy };         // 左
        out[1] = { cx, cy - hh };         // 上
        out[2] = { cx + hw, cy };         // 右
        out[3] = { cx, cy + hh };         // 下
    }
};