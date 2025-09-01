#include "GameMode.hpp"

#include "Sprites.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

#include <bitset>
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

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
	player_idle = &ret.lookup("player_up");
	player_idle = &ret.lookup("player_down");
	player_idle = &ret.lookup("player_left");
	player_idle = &ret.lookup("player_right");

	player_idle = &ret.lookup("flower");

	player_idle = &ret.lookup("background");
	player_idle = &ret.lookup("void");

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
	ppu.sprites[PLAYER].index = sprites.value->sprites.at("player").tiles[0].tile_index;
	ppu.sprites[PLAYER].attributes = sprites.value->sprites.at("player").tiles[0].palette_index;

	std::cout << int(ppu.sprites[PLAYER].index) << std::endl;
	std::cout << int(ppu.sprites[PLAYER].attributes) << std::endl;

	ppu.sprites[PLAYER_UP].index = sprites.value->sprites.at("player_up").tiles[0].tile_index;
	ppu.sprites[PLAYER_UP].attributes = sprites.value->sprites.at("player_up").tiles[0].palette_index;

	std::cout << int(ppu.sprites[PLAYER_UP].index) << std::endl;
	std::cout << int(ppu.sprites[PLAYER_UP].attributes) << std::endl;

	ppu.sprites[PLAYER_DOWN].index = sprites.value->sprites.at("player_down").tiles[0].tile_index;
	ppu.sprites[PLAYER_DOWN].attributes = sprites.value->sprites.at("player_down").tiles[0].palette_index;

	ppu.sprites[PLAYER_LEFT].index = sprites.value->sprites.at("player_left").tiles[0].tile_index;
	ppu.sprites[PLAYER_LEFT].attributes = sprites.value->sprites.at("player_left").tiles[0].palette_index;

	ppu.sprites[PLAYER_RIGHT].index = sprites.value->sprites.at("player_right").tiles[0].tile_index;
	ppu.sprites[PLAYER_RIGHT].attributes = sprites.value->sprites.at("player_right").tiles[0].palette_index;

	ppu.sprites[BACKGROUND].index = sprites.value->sprites.at("background").tiles[0].tile_index;
	ppu.sprites[BACKGROUND].attributes = sprites.value->sprites.at("background").tiles[0].palette_index;

	ppu.sprites[VOID].index = sprites.value->sprites.at("void").tiles[0].tile_index;
	ppu.sprites[VOID].attributes = sprites.value->sprites.at("void").tiles[0].palette_index;

	int nb_tiles = 0;
	for (Sprite::TileRef tile_ref : sprites.value->sprites.at("flower").tiles)
	{
		ppu.sprites[FLOWER + nb_tiles].index = tile_ref.tile_index;
		ppu.sprites[FLOWER + nb_tiles].attributes = tile_ref.palette_index;
		ppu.sprites[FLOWER + nb_tiles].x = 0;
		ppu.sprites[FLOWER + nb_tiles].y = 240;
		nb_tiles++;
	}
	// int i = 0;
	for (unsigned int i = 0; i < ppu.BackgroundWidth * ppu.BackgroundHeight; i++)
	{
		ppu.background[i] = 12 + (4 << 8);
		// std::cout << sprites.value->sprites.at("background").tiles[0].tile_index << std::endl;
		// i = (i + 1) % 13;
	}

	// for (size_t j = 0; j < 13; j ++){
	// 	std::cout << " Tile : " << j << std::endl;
	// 	for (int i = 7; i >= 0; i--)
	// 	{
	// 		std::bitset<8> x(int(ppu.tile_table[j].bit1[i]));
	// 		std::bitset<8> y(int(ppu.tile_table[j].bit0[i]));
	// 		for (int k = 0; k < 8; k++) {
	// 			int palette_index = (x[k] << 1) + y[k];
	// 			if (palette_index == 0) {
	// 				std::cout << ANSI_COLOR_GREEN;
	// 			}
	// 			if (palette_index == 1) {
	// 				std::cout << ANSI_COLOR_BLUE;
	// 			}
	// 			if (palette_index == 2) {
	// 				std::cout << ANSI_COLOR_RED;
	// 			}
	// 			if (palette_index == 3) {
	// 				std::cout << ANSI_COLOR_YELLOW;
	// 			}
	// 			std::cout << "◼" << " " << ANSI_COLOR_RESET;;
	// 		}
	// 		std::cout << std::endl;
	// 	}
	// }

	// for (int i = 0; i < FLOWER + 6; i ++) {
	// 	std::cout << "Palette : " << int(ppu.sprites[i].attributes) << std::endl;
	// 	std::cout << "Tile : " << int(ppu.sprites[i].index) << std::endl;
	// }

	// for (PPU466::Palette palette : palette_table)
	// {
	// 	for (int j = 0; j < 4; j++)
	// 	{
	// 		for (int i = 0; i < 4; i++)
	// 		{
	// 			std::cout << int(palette[j][i]) << " ";
	// 		}
	// 		std::cout << std::endl;
	// 	}
	// 	std::cout << "=========================" << std::endl;
	// }
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

	// slowly rotates through [0,1):
	//  (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

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
	//--- set ppu state based on game state ---

	// background color will be some hsv-like fade:
	// ppu.background_color = glm::u8vec4(
	// 	std::min(255, std::max(0, int32_t(255 * 0.5f * (0.5f + std::sin(2.0f * M_PI * (background_fade + 0.0f / 3.0f)))))),
	// 	std::min(255, std::max(0, int32_t(255 * 0.5f * (0.5f + std::sin(2.0f * M_PI * (background_fade + 1.0f / 3.0f)))))),
	// 	std::min(255, std::max(0, int32_t(255 * 0.5f * (0.5f + std::sin(2.0f * M_PI * (background_fade + 2.0f / 3.0f)))))),
	// 	0xff);

	// tilemap gets recomputed every frame as some weird plasma thing:
	// NOTE: don't do this in your game! actually make a map or something :-)
	//  for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
	//  	for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
	//  		//TODO: make weird plasma thing
	//  		ppu.background[x+PPU466::BackgroundWidth*y] = ((x+y)%16);
	//  	}
	//  }

	// //background scroll:
	// ppu.background_position.x = int32_t(-0.5f * player_at.x);
	// ppu.background_position.y = int32_t(-0.5f * player_at.y);

	// player sprite:
	ppu.sprites[PLAYER].x = int8_t(player_at.x);
	ppu.sprites[PLAYER].y = int8_t(player_at.y);
	// ppu.sprites[0].index = 32;
	// ppu.sprites[0].attributes = 7;

	// some other misc sprites:
	//  for (uint32_t i = 1; i < 63; ++i) {
	//  	float amt = (i + 2.0f * background_fade) / 62.0f;
	//  	ppu.sprites[i].x = int8_t(0.5f * float(PPU466::ScreenWidth) + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * float(PPU466::ScreenWidth));
	//  	ppu.sprites[i].y = int8_t(0.5f * float(PPU466::ScreenHeight) + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * float(PPU466::ScreenWidth));
	//  	ppu.sprites[i].index = 32;
	//  	ppu.sprites[i].attributes = 6;
	//  	if (i % 2) ppu.sprites[i].attributes |= 0x80; //'behind' bit
	//  }

	//--- actually draw ---
	ppu.draw(drawable_size);
}
