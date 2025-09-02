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

#include <bitset>
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

// Load all the game sprites into memory
Sprite const *player_idle = nullptr;
Sprite const *player_up = nullptr;
Sprite const *player_down = nullptr;
Sprite const *player_left = nullptr;
Sprite const *player_right = nullptr;

Sprite const *flower = nullptr;
Sprite const *void_puddle = nullptr;

Sprite const *background_tile1 = nullptr;
Sprite const *background_tile2 = nullptr;
Sprite const *background_tile3 = nullptr;
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
	void_puddle = &ret.lookup("void_puddle");

	background_tile1 = &ret.lookup("background1");
	background_tile2 = &ret.lookup("background2");
	background_tile3 = &ret.lookup("background3");
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

	setup_map(default_flowers, default_puddles, default_death_time);
}

GameMode::~GameMode()
{
}

void GameMode::setup_map(uint8_t nb_flowers, uint8_t nb_puddles, uint16_t death_time)
{
	current_flowers = nb_flowers;
	current_puddles = nb_puddles;
	death_timer = death_time;

	flowers = {};
	void_puddles = {};

	srand((unsigned)time(NULL));

	int nb_tiles = FLOWER;
	for (int i = 0; i < nb_flowers; i++)
	{
		flowers.push_back(nb_tiles);
		int flower_x = rand() % (256 - 3 * tile_size);
		int flower_y = rand() % (240 - 2 * tile_size);
		for (Sprite::TileRef tile_ref : flower->tiles)
		{
			ppu.sprites[nb_tiles].index = tile_ref.tile_index;
			ppu.sprites[nb_tiles].attributes = tile_ref.palette_index;
			ppu.sprites[nb_tiles].x = flower_x + tile_ref.offset_x_chunk * 8;
			ppu.sprites[nb_tiles].y = flower_y + tile_ref.offset_y_chunk * 8;
			nb_tiles++;
		}
	}

	for (int i = 0; i < nb_puddles; i++)
	{
		void_puddles.push_back(nb_tiles);
		int puddle_x = rand() % (256 - 2 * tile_size);
		int puddle_y = rand() % (240 - 2 * tile_size);
		for (Sprite::TileRef tile_ref : void_puddle->tiles)
		{
			ppu.sprites[nb_tiles].index = tile_ref.tile_index;
			ppu.sprites[nb_tiles].attributes = tile_ref.palette_index;
			ppu.sprites[nb_tiles].x = puddle_x + tile_ref.offset_x_chunk * 8;
			ppu.sprites[nb_tiles].y = puddle_y + tile_ref.offset_y_chunk * 8;
			nb_tiles++;
		}
	}

	// Setting the background to the background tiles
	Sprite const *background_sprites[3] = {background_tile1, background_tile2, background_tile3};
	for (unsigned int i = 0; i < ppu.BackgroundWidth * ppu.BackgroundHeight; i++)
	{
		int rand_background = rand() % 3;
		ppu.background[i] = background_sprites[rand_background]->tiles[0].tile_index + (background_sprites[rand_background]->tiles[0].palette_index << 8);
	}
}

bool GameMode::colide(uint8_t obj1_x, uint8_t obj1_y, uint8_t obj1_size, uint8_t obj2_x, uint8_t obj2_y, uint8_t obj2_size)
{
	// Collision with bottom left corner
	return (obj1_x >= obj2_x && obj1_x <= obj2_x + obj2_size &&
			obj1_y >= obj2_y && obj1_y <= obj2_y + obj2_size) ||
		   // Collision with bottom right corner
		   (obj1_x + obj1_size >= obj2_x && obj1_x + obj1_size <= obj2_x + obj2_size &&
			obj1_y >= obj2_y && obj1_y <= obj2_y + obj2_size) ||
		   // Collision with top left corner
		   (obj1_x >= obj2_x && obj1_x <= obj2_x + obj2_size &&
			obj1_y + obj1_size >= obj2_y && obj1_y + obj1_size <= obj2_y + obj2_size) ||
		   // Collision with top right corner
		   (obj1_x + obj1_size >= obj2_x && obj1_x + obj1_size <= obj2_x + obj2_size &&
			obj1_y + obj1_size >= obj2_y && obj1_y + obj1_size <= obj2_y + obj2_size);
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
		else if (evt.key.key == SDLK_SPACE)
		{
			space.pressed = true;
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
		else if (evt.key.key == SDLK_SPACE)
		{
			space.pressed = false;
			return true;
		}
	}

	return false;
}

void GameMode::update(float elapsed)
{
	death_timer--;
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

	if (space.pressed)
	{
		round = 0;
		setup_map(default_flowers, default_puddles, default_death_time);
	}

	// Make sure the player does not leave the screen
	player_at.x = std::min(player_at.x, float(ppu.ScreenWidth - player_size));
	player_at.x = std::max(player_at.x, 0.0f);

	player_at.y = std::min(player_at.y, float(ppu.ScreenHeight - player_size));
	player_at.y = std::max(player_at.y, 0.0f);

	// Check collision between flowers and the player
	for (int flower_index : flowers)
	{
		for (size_t i = 0; i < flower->tiles.size(); i++)
		{
			// Don't check if the player is colliding with off screen flowers
			if (ppu.sprites[flower_index + i].y == 240)
			{
				break;
			}
			// If the player collides with a flower tile, move the whole flower offscreen
			if (colide(player_at.x, player_at.y, player_size, ppu.sprites[flower_index + i].x, ppu.sprites[flower_index + i].y, tile_size))
			{
				for (size_t collided_flower_index = flower_index; collided_flower_index - flower_index < flower->tiles.size(); collided_flower_index++)
				{
					ppu.sprites[collided_flower_index].y = 240;
				}
				current_flowers--;
				break;
			}
		}
	}

	for (int puddle_index : void_puddles)
	{
		for (size_t i = 0; i < void_puddle->tiles.size(); i++)
		{
			// Don't check if the player is colliding with off screen puddles
			if (ppu.sprites[puddle_index + i].y == 240)
			{
				break;
			}
			// If the player collides with a puddle tile, move the whole puddle offscreen and add time to counter
			if (colide(player_at.x, player_at.y, player_size, ppu.sprites[puddle_index + i].x, ppu.sprites[puddle_index + i].y, tile_size))
			{
				for (size_t collided_puddle_index = puddle_index; collided_puddle_index - puddle_index < void_puddle->tiles.size(); collided_puddle_index++)
				{
					ppu.sprites[collided_puddle_index].y = 240;
					current_puddles--;
					death_timer += 60 * 2;
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

	if (current_flowers == 0)
	{
		round++;
		setup_map(default_flowers + round, std::max(default_puddles - round, 0), default_death_time - 60 * round);
	}
	if (death_timer <= 0)
	{
		for (unsigned int i = 0; i < ppu.BackgroundWidth * ppu.BackgroundHeight; i++)
		{
			ppu.background[i] = void_tile->tiles[0].tile_index + (void_tile->tiles[0].palette_index << 8);
		}
		for (int puddle_index : void_puddles)
		{
			for (size_t i = 0; i < void_puddle->tiles.size(); i++)
			{
				ppu.sprites[puddle_index + i].y = 240;
				death_timer += 60 * 2;
			}

			for (int flower_index : flowers)
			{
				for (size_t i = 0; i < flower->tiles.size(); i++)
				{
					ppu.sprites[flower_index + i].y = 240;
				}
			}
		}
	}

	ppu.draw(drawable_size);
}
