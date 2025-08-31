#include "Sprites.hpp"
#include <read_write_chunk.hpp>
#include <fstream>

#include <vector>

Sprites Sprites::load(std::string const &filename)
{
    std::ifstream file(filename, std::ios::binary);

    std::vector<char> names;
    std::vector<Sprite::TileRef> refs;
    struct StoredSprite
    {
        uint32_t name_begin, name_end;
        uint32_t refs_begin, refs_end;
    };
    std::vector<StoredSprite> sprites;

    read_chunk(file, "name", &names);
    read_chunk(file, "refs", &refs);
    read_chunk(file, "sprt", &sprites);

    char junk;
    if (file.read(&junk, 1))
    {
        throw std::runtime_error("Trailing junk at the end of the file");
    }

    Sprites ret;

    for (StoredSprite const &stored : sprites)
    {
        if (!(stored.name_begin < stored.name_end && stored.name_end <= uint32_t(names.size())))
        {
            throw std::runtime_error("Sprite with bad name range.");
        }
        if (!(stored.refs_begin < stored.refs_end && stored.refs_end <= uint32_t(refs.size())))
        {
            throw std::runtime_error("Sprite with bad refs range.");
        }

        std::string name(
            names.begin() + stored.name_begin,
            names.begin() + stored.name_end);
        Sprite sprite;

        sprite.tiles.assign(
            refs.begin() + stored.refs_begin,
            refs.end() + stored.refs_end);

        auto result = ret.sprites.emplace(name, sprite);
        if (!result.second)
        {
            throw std::runtime_error("Two sprites with the same name ('" + name + "')");
        }
    }

    return ret;
}