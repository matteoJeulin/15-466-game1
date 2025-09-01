#include "parse_ppm.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>

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

// Define the indexes in a colour
constexpr int RED = 0;
constexpr int GREEN = 1;
constexpr int BLUE = 2;
constexpr int ALPHA = 3;

void PPM_Parser::parse_chunk(std::string const &filename)
{
    std::ifstream file(filename);

    // Palette for this sprite
    PPU466::Palette palette;

    // Tile this image will be represented as
    PPU466::Tile tile;

    // Scrambling palette so black can be registered
    palette[0][0] = 0xff;
    palette[1][0] = 0xff;
    palette[2][0] = 0xff;
    palette[3][0] = 0xff;

    // Set all alpha colours to 0. If alpha is 0
    // the colour hasn't yet been set in this palette
    palette[0][ALPHA] = 0;
    palette[1][ALPHA] = 0;
    palette[2][ALPHA] = 0;
    palette[3][ALPHA] = 0;

    long unsigned int colours_registered = 0;
    bool to_register = true;

    // Indicates what the index of the current pixel's colour is in the palette
    long unsigned int colour_index = 0;

    int row = chunk_size - 1;
    int column = 0;
    int pixel_count = 0;

    int R;
    int G;
    int B;
    while (file >> R >> G >> B)
    {
        // std::cout << R << " " << G << " " << B << std::endl;
        // Check if the current colour has already been registered in the palette
        for (long unsigned int i = 0; i <= colours_registered && i < palette.size(); i++)
        {
            // If it has, exit the loop
            if (R == palette[i][RED] && G == palette[i][GREEN] && B == palette[i][BLUE])
            {
                colour_index = i;
                to_register = false;
                break;
            }
        }

        // If there is still space in the current palette and a new colour
        // has been found, add it in the palette.
        if (colours_registered < palette.size() && to_register)
        {
            palette[colours_registered][RED] = R;
            palette[colours_registered][GREEN] = G;
            palette[colours_registered][BLUE] = B;
            palette[colours_registered][ALPHA] = 0xff;

            colour_index = colours_registered;

            colours_registered++;
        }
        to_register = true;
    }

    // Search if the palette has already been registered and get its index in the palette table (default is last spot)
    // Also check if the current palette is contained in or contains another palette (using the alpha channel)
    uint16_t palette_index = palette_table.size();
    bool palette_found = false;
    for (size_t i = 0; i < palette_table.size(); i++)
    {
        int nb_colours_found = 0;
        for (glm::u8vec4 colour : palette)
        {
            // If the colour we are checking has an alpha channel that is null,
            // it means we found all the previous colours of palette in the other
            // palette and that palette is contained in the other palette.
            if (colour[ALPHA] == 0)
            {
                nb_colours_found = palette.size();
                break;
            }
            bool colour_found = false;
            for (glm::u8vec4 &registered_colour : palette_table[i])
            {
                if (colour[RED] == registered_colour[RED] && colour[GREEN] == registered_colour[GREEN] && colour[BLUE] == registered_colour[BLUE] && colour[ALPHA] == registered_colour[ALPHA])
                {
                    colour_found = true;
                    colour[ALPHA] = 0xff;
                    break;
                }
                // if (registered_colour[ALPHA] == 0) {
                //     colour[ALPHA] = 0xff;
                //     registered_colour = colour;
                //     colour_found = true;
                //     break;
                // }
            }
            if (!colour_found)
            {
                break;
            }
            else
            {
                nb_colours_found++;
                colour_found = false;
            }
        }
        if (nb_colours_found == palette.size())
        {
            palette_index = i;
            palette_found = true;
            break;
        }
        else
        {
            nb_colours_found = 0;
        }
    }
    if (!palette_found)
    {
        palette_table.push_back(palette);
        palette_index = palette_table.size() - 1;
    }

    file.close();
    file.open(filename);
    // Make sure to only parse the pixels in the image
    while (file >> R >> G >> B && pixel_count < chunk_size * chunk_size)
    {
        // std::cout << R << " " << G << " " << B << std::endl;
        // Find the current colour in the palette
        for (long unsigned int i = 0; i <= colours_registered && i < palette.size(); i++)
        {
            if (R == palette[i][RED] && G == palette[i][GREEN] && B == palette[i][BLUE])
            {
                colour_index = i;
                break;
            }
        }

        // Add the pixel to the tile
        tile.bit0[row] += (colour_index % 2) << column;
        tile.bit1[row] += ((colour_index >> 1) % 2) << column;
        // std::cout << row << " " << column << std::endl;
        // std::bitset<8> bit1(tile.bit1[row]);
        // std::bitset<8> bit0(tile.bit0[row]);
        // std::cout << bit1 << " " << bit0 << std::endl;
        pixel_count++;
        column = (column + 1);
        row -= column / chunk_size;
        column %= chunk_size;
    }

    for (int i = 7; i >= 0; i--)
		{
			std::bitset<8> x(int(tile.bit1[i]));
			std::bitset<8> y(int(tile.bit0[i]));
			for (int k = 0; k < 8; k++) {
				int palette_index = (x[k] << 1) + y[k];
				if (palette_index == 0) {
					std::cout << ANSI_COLOR_GREEN;
				}
				if (palette_index == 1) {
					std::cout << ANSI_COLOR_BLUE;
				}
				if (palette_index == 2) {
					std::cout << ANSI_COLOR_RED;
				}
				if (palette_index == 3) {
					std::cout << ANSI_COLOR_MAGENTA;
				}
				// std::cout << x[k] << y[k] << " " << ANSI_COLOR_RESET;
				std::cout << "◼" << " " << ANSI_COLOR_RESET;;
			}
			std::cout << "Row : " << i <<std::endl;
		}

    // Constructing the tile ref for our new tile
    Sprite::TileRef tile_ref;
    tile_ref.palette_index = palette_index;
    std::cout << "Palette : " << palette_index << std::endl;
    tile_ref.tile_index = tile_table.size();
    std::cout << "Tile : " << tile_ref.tile_index << std::endl;
    tile_ref.offset_x_chunk = 0; // Is set to 0 by default
    tile_ref.offset_y_chunk = 0; // Modified in parse_image if needed

    tile_refs.push_back(tile_ref);

    tile_table.push_back(tile);
}

void PPM_Parser::parse_image(std::string const &filename, std::string const &output_file)
{
    std::ifstream in_file(filename);
    std::ofstream out_file(output_file, std::ios_base::app);

    char trash[2];
    in_file.read(trash, 2); // Read the header of the ppm file (P3)

    int width, height, colours;
    in_file >> width >> height >> colours;

    // Number of chunks in the image
    uint8_t chunks_in_row = width % chunk_size == 0 ? (width / chunk_size) : (width / chunk_size) + 1;
    uint8_t chunks_in_column = height % chunk_size == 0 ? (height / chunk_size) : (height / chunk_size) + 1;

    // std::cout << "================================================" << std::endl;

    // std::cout << "Initial values : " << int(chunks_in_row) << " " << int(chunks_in_column) << std::endl;

    // Coordinates in the image's chunks
    uint8_t chunks_parsed_row = 0;
    uint8_t chunks_parsed_column = 0;

    // Pixels seen since the begining of the line in the current chunk
    uint8_t pixels_seen = 0;
    // Lines passed since the begining of the parsing operation on the current chunk
    uint8_t lines_passed = 0;

    // Go through the image and write it chunk by chunk into the parsing file
    // so that each chunk gets parsed individually
    int R, G, B;
    while (chunks_parsed_column < chunks_in_column)
    {
        // std::cout << "Start of loop: Chunks parsed column " << int(chunks_parsed_column) << std::endl;
        in_file >> R >> G >> B;

        if (pixels_seen < chunk_size)
        {
            out_file << R << " " << G << " " << B << std::endl;
            // std::cout << "Writing pixel " << int(pixels_seen) << " " << int(lines_passed) << std::endl;
            pixels_seen += 1;
        }
        if (pixels_seen >= chunk_size)
        {
            lines_passed += 1;
            // Number of pixels until new line in chunk
            uint8_t pixels_to_chunk = width - chunk_size;
            // Navigate to new chunk line
            while (pixels_to_chunk > 0 && in_file >> R >> G >> B)
            {
                pixels_to_chunk--;
            }
            pixels_seen = 0;
            // std::cout << "Going to new chunk line" << std::endl;
        }
        if (lines_passed >= chunk_size)
        {
            // std::cout << "Parsing chunk !!" << std::endl << std::endl;
            parse_chunk(output_file);

            tile_refs.back().offset_x_chunk = chunks_parsed_row;
            // The file is read from top left to bottom right
            // but the sprite is displayed from bottom left to top right
            tile_refs.back().offset_y_chunk = height - chunks_parsed_column;
            // std::cout << "Done parsing chunk !!" << std::endl << std::endl;

            chunks_parsed_row += 1;
            lines_passed = 0;

            // Empty all the text from the output file to make space for the new chunk.
            out_file.close();
            std::remove(output_file.c_str());
            out_file.open(output_file, std::ios_base::app);

            // Don't need to move the cursor if we've just finished a line
            if (chunks_parsed_row != chunks_in_row)
            {
                // Re-open the file and start from the begining
                in_file.close();
                in_file.open(filename);

                char trash[2];
                in_file.read(trash, 2); // Read the header of the ppm file (P3)

                int width, height, colours;
                in_file >> width >> height >> colours;

                // Number of pixels until new line in chunk
                uint8_t pixels_to_chunk = width * chunks_parsed_column + chunk_size * chunks_parsed_row;

                // Navigate to new chunk line
                while (pixels_to_chunk > 0 && in_file >> R >> G >> B)
                {
                    pixels_to_chunk--;
                }

                // std::cout << "Going to new chunk " << int(chunks_parsed_row) << std::endl;
            }
        }
        if (chunks_parsed_row >= chunks_in_row)
        {
            chunks_parsed_column += 1;
            chunks_parsed_row = 0;

            // std::cout << "End of row " << int(chunks_parsed_column) << std::endl;
        }
    }

    std::string sprite_name = std::filesystem::path(filename).stem();

    std::ofstream output("./parsing/sprites/" + sprite_name + ".ppu", std::ios::binary);
    write_chunk("refs", tile_refs, &output);
}

void PPM_Parser::parse_directory(std::string const &filename)
{
    for (const auto &entry : std::filesystem::recursive_directory_iterator(filename))
    {
        std::cout << entry.path() << std::endl;
        parse_image(entry.path(), "parsing/tmp_chunk.txt");
    }

    std::ofstream output("./parsing/tables.ppu");

    // Writing the parsed data to files to be read by the game
    write_chunk("tile", tile_table, &output);
    write_chunk("palt", palette_table, &output);
}

int main()
{
    PPM_Parser parser;
    // parser.parse_image("./sprites/player.ppm", "parsing/tmp_chunk.txt");
    // parser.parse_image("./sprites/player_up.ppm", "parsing/tmp_chunk.txt");
    // parser.parse_image("./sprites/player_down.ppm", "parsing/tmp_chunk.txt");
    // parser.parse_image("./sprites/player_left.ppm", "parsing/tmp_chunk.txt");
    // parser.parse_image("./sprites/player_right.ppm", "parsing/tmp_chunk.txt");

    // parser.parse_image("./sprites/background.ppm", "parsing/tmp_chunk.txt");
    // parser.parse_image("./sprites/void.ppm", "parsing/tmp_chunk.txt");

    // parser.parse_image("./sprites/flower.ppm", "parsing/tmp_chunk.txt");
    parser.parse_directory("./sprites");
}
