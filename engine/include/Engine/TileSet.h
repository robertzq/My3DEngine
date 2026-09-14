#pragma once
#include <map>
#include <string>

struct TileDef {
    std::string texture;
    bool solid = false;
    bool overlay = false;
    std::string trigger;
};

class TileSet {
public:
    int tileSize = 32;
    std::map<int, TileDef> tiles;

    const TileDef* Get(int id) const {
        auto it = tiles.find(id);
        return it == tiles.end() ? nullptr : &it->second;
    }

    bool IsSolid(int id) const {
        const TileDef* def = Get(id);
        return def != nullptr && def->solid;
    }

    bool IsOverlay(int id) const {
        const TileDef* def = Get(id);
        return def != nullptr && def->overlay;
    }

    std::string TriggerOf(int id) const {
        const TileDef* def = Get(id);
        return def != nullptr ? def->trigger : std::string();
    }
};
