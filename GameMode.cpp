#include "GameMode.hpp"

#include "Sprites.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

// for the GL_ERRORS() macro:
#include "gl_errors.hpp"

// for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>

#include <random>

// Load all the game sprites into memory
Sprite const *player_idle = nullptr;
Sprite const *player_up = nullptr;
Sprite const *player_down = nullptr;
Sprite const *player_left = nullptr;
Sprite const *player_right = nullptr;

Sprite const *flower = nullptr;

Sprite const *background_tile = nullptr;
Sprite const *void_tile = nullptr;

Load<Sprites> sprites(LoadTagDefault, []() -> Sprites const *
					  {
	static Sprites ret = Sprites::load(data_path("../parsing/sprites"));

	player_idle = &ret.lookup("player");
	player_up = &ret.lookup("player_up");
	player_down = &ret.lookup("player_down");
	player_left = &ret.lookup("player_left");
	player_right = &ret.lookup("player_right");

	flower = &ret.lookup("flower");

	background_tile = &ret.lookup("background");
	void_tile = &ret.lookup("void");

	return &ret; });

// Define the indexes of the first tile of each sprite in the sprite table
constexpr int PLAYER = 0;
constexpr int PLAYER_UP = 1;
constexpr int PLAYER_DOWN = 2;
constexpr int PLAYER_LEFT = 3;
constexpr int PLAYER_RIGHT = 4;

constexpr int BACKGROUND = 5;
constexpr int VOID = 6;

constexpr int FLOWER = 7;

GameMode::GameMode()
{
	// Updating palette table and tile table
	std::vector<PPU466::Palette> palette_table;
	std::vector<PPU466::Tile> tile_table;

	std::ifstream file(data_path("../parsing/tables.ppu"));

	read_chunk(file, "tile", &tile_table);
	read_chunk(file, "palt", &palette_table);

	for (size_t i = 0; i < palette_table.size(); i++)
	{
		ppu.palette_table[i] = palette_table[i];
	}

	for (size_t i = 0; i < tile_table.size(); i++)
	{
		ppu.tile_table[i] = tile_table[i];
	}

	// Put all the game sprites into the sprite table
	ppu.sprites[PLAYER].index = player_idle->tiles[0].tile_index;
	ppu.sprites[PLAYER].attributes = player_idle->tiles[0].palette_index;

	ppu.sprites[PLAYER_UP].index = player_up->tiles[0].tile_index;
	ppu.sprites[PLAYER_UP].attributes = player_up->tiles[0].palette_index;

	ppu.sprites[PLAYER_DOWN].index = player_down->tiles[0].tile_index;
	ppu.sprites[PLAYER_DOWN].attributes = player_down->tiles[0].palette_index;

	ppu.sprites[PLAYER_LEFT].index = player_left->tiles[0].tile_index;
	ppu.sprites[PLAYER_LEFT].attributes = player_left->tiles[0].palette_index;

	ppu.sprites[PLAYER_RIGHT].index = player_right->tiles[0].tile_index;
	ppu.sprites[PLAYER_RIGHT].attributes = player_right->tiles[0].palette_index;

	ppu.sprites[BACKGROUND].index = background_tile->tiles[0].tile_index;
	ppu.sprites[BACKGROUND].attributes = background_tile->tiles[0].palette_index;

	ppu.sprites[VOID].index = void_tile->tiles[0].tile_index;
	ppu.sprites[VOID].attributes = void_tile->tiles[0].palette_index;

	int nb_tiles = 0;
	for (Sprite::TileRef tile_ref : flower->tiles)
	{
		ppu.sprites[FLOWER + nb_tiles].index = tile_ref.tile_index;
		ppu.sprites[FLOWER + nb_tiles].attributes = tile_ref.palette_index;
		ppu.sprites[FLOWER + nb_tiles].x = 0;
		ppu.sprites[FLOWER + nb_tiles].y = 240;
		nb_tiles++;
	}

	for (unsigned int i = 0; i < ppu.BackgroundWidth * ppu.BackgroundHeight; i++)
	{
		ppu.background[i] = 12 + (4 << 8);
	}
}

GameMode::~GameMode()
{
}

bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

	if (evt.type == SDL_EVENT_KEY_DOWN)
	{
		if (evt.key.key == SDLK_LEFT)
		{
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_RIGHT)
		{
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_UP)
		{
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_DOWN)
		{
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_KEY_UP)
	{
		if (evt.key.key == SDLK_LEFT)
		{
			left.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_RIGHT)
		{
			right.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_UP)
		{
			up.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_DOWN)
		{
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void GameMode::update(float elapsed)
{
	constexpr float PlayerSpeed = 30.0f;
	if (left.pressed)
		player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed)
		player_at.x += PlayerSpeed * elapsed;
	if (down.pressed)
		player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed)
		player_at.y += PlayerSpeed * elapsed;

	// reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void GameMode::draw(glm::uvec2 const &drawable_size)
{
	// player sprite:
	ppu.sprites[PLAYER].x = int8_t(player_at.x);
	ppu.sprites[PLAYER].y = int8_t(player_at.y);

	ppu.draw(drawable_size);
}
