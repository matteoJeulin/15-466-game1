#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct GameMode : Mode {
	GameMode();
	virtual ~GameMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	// Vector containing all the indexes of the first tile of all
	// flower sprites in the tile table
	std::vector<int> flowers;

	// Number of flowers in game state
	uint8_t const default_flowers = 5;
	uint8_t current_flowers = default_flowers;

	// Vector containing all the indexes of the first tile of all
	// void puddle in the tile table
	std::vector<int> void_puddles;

	// Number of void puddles in game state
	uint8_t const default_puddles = 5;
	uint8_t current_puddles = default_puddles;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f);

	// Default tile sizes for tiles in the game
	uint8_t const tile_size = 8;
	uint8_t const player_size = tile_size;

	// Timer to see when player dies
	uint16_t const default_death_time = 60 * 10;
	uint16_t death_timer = default_death_time;

	// Number of rounds played
	uint16_t round = 0;

	// Function to detect the collision between two objects based on their position and size
	// Both objects should be squares, it is meant to be used on tiles
	static bool colide(uint8_t obj1_x, uint8_t obj1_y, uint8_t obj1_size, uint8_t obj2_x, uint8_t obj2_y, uint8_t obj2_size);

	// Sets up the map
	void setup_map(uint8_t nb_flowers, uint8_t nb_puddles, uint16_t death_time);

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
