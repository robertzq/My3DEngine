#pragma once
#include <SDL.h>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "Engine/TileSet.h"

class Texture;

struct TileSprite {
    SDL_Rect rect;
    Texture* texture = nullptr;   // non-owning
    int sortY = 0;
};

class TileMap {
public:
    bool Load(const std::string& mapResourceId, const TileSet& tileSet);

    void Draw(const SDL_Rect& camera) const;
    void DrawGround(const SDL_Rect& camera) const;

    const std::vector<SDL_Rect>& Colliders() const { return colliders; }
    std::vector<SDL_Rect> TilesWithId(int id) const;
    const std::vector<std::pair<std::string, SDL_Rect>>& Triggers() const { return triggers; }
    std::vector<SDL_Rect> TriggerRects(const std::string& name) const;
    const std::vector<TileSprite>& Overlays() const { return overlays; }

    void SetTile(int col, int row, int id);

    bool Empty() const { return data.empty(); }
    int Width() const { return width; }
    int Height() const { return height; }
    int TileSize() const { return tileSize; }
    int WidthPx() const { return width * tileSize; }
    int HeightPx() const { return height * tileSize; }

private:
    void RebuildMetadata();
    void DrawLayer(const SDL_Rect& camera, bool overlayLayer) const;

    int tileSize = 32;
    int width = 0;
    int height = 0;
    std::vector<std::vector<int>> data;
    TileSet tileSet;
    std::map<int, Texture*> textures;
    std::vector<SDL_Rect> colliders;
    std::vector<std::pair<std::string, SDL_Rect>> triggers;
    std::vector<TileSprite> overlays;
};
