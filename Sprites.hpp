#pragma once

#include <string>
#include <stdint.h>
#include <map>
#include <vector>

struct Sprite
{

    struct TileRef
    {
        uint16_t tile_index = 0;
        uint16_t palette_index = 0;
        // Position relative to the bottom left tile in the sprite (in chunks)
        int16_t offset_x_chunk = 0;
        int16_t offset_y_chunk = 0;
    };
    static_assert(sizeof(TileRef) == 8, "TileRef doesn't contain padding bytes.");

    std::vector<TileRef> tiles;

    std::string name;

    // Draw sprite with origin at x,y
    void draw(int32_t x, int32_t y) const;

    // Load sprite from the given filepath
    static Sprite load(std::string const &filename);
};

struct Sprites
{
    // Look up a specific sprite by name  and
    // return a reference to it (or throw an error if failure)
    Sprite const &lookup(std::string const &name);

    // Load sprites from the given filepath
    static Sprites load(std::string const &filename);

    std::map<std::string, Sprite> sprites;
};
