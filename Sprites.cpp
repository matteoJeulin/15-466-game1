#include "Sprites.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

#include "read_write_chunk.hpp"
#include "PPU466.hpp"


Sprite Sprite::load(std::string const &filename)
{

    std::ifstream file(filename, std::ios::binary);

    Sprite sprite;

    read_chunk(file, "refs", &sprite.tiles);

    sprite.name = std::filesystem::path(filename).stem();

    return sprite;
}

void Sprite::draw(int32_t x, int32_t y) const {

}

Sprites Sprites::load(std::string const &filename)
{
    Sprites ret;
    // Load all the sprites in a directory
    for (const auto &entry : std::filesystem::recursive_directory_iterator(filename))
    {
        ret.sprites.emplace(entry.path().stem(), Sprite::load(entry.path()));
    }
    return ret;
}

Sprite const &Sprites::lookup(std::string const &name) {
    return sprites.at(name);
}