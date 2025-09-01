#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <map>

#include "PPU466.hpp"
#include "Sprites.hpp"

struct PPM_Parser
{
    std::vector<PPU466::Palette> palette_table;
    std::vector<PPU466::Tile> tile_table;
    std::vector<Sprite::TileRef> tile_refs;

    uint8_t chunk_size = 8;

    // Parses an 8x8 chunk of pixels in PPM P3 format (RGB 8 bit colours)
    void parse_chunk(std::string const &filename);

    // Takes a given PPM P3 image, parses it and writes the resulting data to out (file name)
    void parse_image(std::string const &filename, std::string const &out);

    // Parse every image in a given directory
    void parse_directory(std::string const &filename);
};