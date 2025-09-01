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
#include <time.h>

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
constexpr int FLOWER = 1;

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

	srand((unsigned)time(NULL));

	int nb_tiles = 0;
	int nb_flowers = 3;
	for (int i = 0; i < nb_flowers; i++)
	{
		flowers.push_back(FLOWER + nb_tiles);
		int flower_x = rand() % 256;
		int flower_y = rand() % 241;
		for (Sprite::TileRef tile_ref : flower->tiles)
		{
			ppu.sprites[FLOWER + nb_tiles].index = tile_ref.tile_index;
			ppu.sprites[FLOWER + nb_tiles].attributes = tile_ref.palette_index;
			ppu.sprites[FLOWER + nb_tiles].x = flower_x + tile_ref.offset_x_chunk * 8;
			ppu.sprites[FLOWER + nb_tiles].y = flower_y + tile_ref.offset_y_chunk * 8;
			nb_tiles++;
		}
	}

	// Setting the background to the background tile
	for (unsigned int i = 0; i < ppu.BackgroundWidth * ppu.BackgroundHeight; i++)
	{
		ppu.background[i] = flower->tiles[1].tile_index + (flower->tiles[1].palette_index << 8);
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
			left.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_RIGHT)
		{
			right.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_UP)
		{
			up.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_DOWN)
		{
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
	// Player movement and animation
	constexpr float PlayerSpeed = 50.0f;
	if (left.pressed)
	{
		player_at.x -= PlayerSpeed * elapsed;
		ppu.sprites[PLAYER].index = player_left->tiles[0].tile_index;
		ppu.sprites[PLAYER].attributes = player_left->tiles[0].palette_index;
	}
	if (right.pressed)
	{
		player_at.x += PlayerSpeed * elapsed;
		ppu.sprites[PLAYER].index = player_right->tiles[0].tile_index;
		ppu.sprites[PLAYER].attributes = player_right->tiles[0].palette_index;
	}
	if (down.pressed)
	{
		player_at.y -= PlayerSpeed * elapsed;
		ppu.sprites[PLAYER].index = player_down->tiles[0].tile_index;
		ppu.sprites[PLAYER].attributes = player_down->tiles[0].palette_index;
	}
	if (up.pressed)
	{
		player_at.y += PlayerSpeed * elapsed;
		ppu.sprites[PLAYER].index = player_up->tiles[0].tile_index;
		ppu.sprites[PLAYER].attributes = player_up->tiles[0].palette_index;
	}

	if (!right.pressed && !left.pressed && !up.pressed && !down.pressed)
	{
		ppu.sprites[PLAYER].index = player_idle->tiles[0].tile_index;
		ppu.sprites[PLAYER].attributes = player_idle->tiles[0].palette_index;
	}

	// Make sure the player does not leave the screen
	player_at.x = std::min(player_at.x, float(ppu.ScreenWidth) - player_size.x);
	player_at.x = std::max(player_at.x, 0.0f);

	player_at.y = std::min(player_at.y, float(ppu.ScreenHeight) - player_size.y);
	player_at.y = std::max(player_at.y, 0.0f);

	// Check collision between flowers and the player
	for (int flower_index : flowers) {
		for (size_t i = 0; i < flower->tiles.size(); i++) {
			// Don't check if the player is colliding with off screen flowers
			if (ppu.sprites[flower_index + i].y == 240) {
				break;
			}
			// If the player collides with a flower tile, move the whole flower offscreen
			if (player_at.x >= ppu.sprites[flower_index + i].x && player_at.x <= ppu.sprites[flower_index + i].x + 8 &&
				player_at.y >= ppu.sprites[flower_index + i].y && player_at.y <= ppu.sprites[flower_index + i].y + 8) {
					for (size_t collided_flower_index = flower_index; collided_flower_index - flower_index < flower->tiles.size(); collided_flower_index++) {
						ppu.sprites[collided_flower_index].y = 240;
					}
					break;
			}
		}
	}
}

void GameMode::draw(glm::uvec2 const &drawable_size)
{
	// Player sprite:
	ppu.sprites[PLAYER].x = int8_t(player_at.x);
	ppu.sprites[PLAYER].y = int8_t(player_at.y);

	ppu.draw(drawable_size);
}
