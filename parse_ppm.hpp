#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <fstream>

#include "PPU466.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

#define RED 0
#define GREEN 1
#define BLUE 2
#define ALPHA 3

struct PPM_Parser {

    std::vector< PPU466::Palette > palettes;
    std::vector< PPU466::Tile > tiles;

    uint8_t chunk_size = 8;

    // Parses an 8x8 chunk of pixels in PPM P3 format (RGB 8 bit colours)
    void parse_chunk(std::string const &filename);

    // Takes a given PPM P3 image, parses it and writes the resulting data to out (file name)
    void parse_image(std::string const &filename, std::string const &out);
};