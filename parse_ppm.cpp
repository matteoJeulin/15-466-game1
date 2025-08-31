#include "parse_ppm.hpp"

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

    palette[0][ALPHA] = 0xff;
    palette[1][ALPHA] = 0xff;
    palette[2][ALPHA] = 0xff;
    palette[3][ALPHA] = 0xff;

    long unsigned int colours_registered = 0;
    bool to_register = true;

    // Indicates what the index of the current pixel's colour is in the palette
    long unsigned int palette_index;

    int row = 7;
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
            // If it hasn't, add it in
            if (R == palette[i][RED] && G == palette[i][GREEN] && B == palette[i][BLUE])
            {
                palette_index = i;
                to_register = false;
                break;
            }
        }

        if (colours_registered < palette.size() && to_register)
        {
            palette[colours_registered][RED] = R;
            palette[colours_registered][GREEN] = G;
            palette[colours_registered][BLUE] = B;

            palette_index = colours_registered;

            colours_registered++;
        }
        to_register = true;

        pixel_count++;
        column = (column + 1);
        row -= column / chunk_size;
        column %= chunk_size;

        tile.bit0[row] += (palette_index % 2) << column;
        tile.bit1[row] += ((palette_index >> 1) % 2) << column;
    }

    palettes.push_back(palette);
    tiles.push_back(tile);
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

    std::cout << "================================================" << std::endl;

    std::cout << "Initial values : " << int(chunks_in_row) << " " << int(chunks_in_column) << std::endl;

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
            chunks_parsed_row += 1;
            lines_passed = 0;

            // std::cout << "Parsing chunk !!" << std::endl << std::endl;
            parse_chunk(output_file);
            // std::cout << "Done parsing chunk !!" << std::endl << std::endl;

            // Empty all the text from the output file to make space for the new chunk. 
            out_file.close();
            out_file.open(output_file, std::ofstream::out | std::ofstream::trunc);
            out_file.close();
            out_file.open(output_file, std::ios_base::app);

            // Don't need to move the cursor if we've just finished a line
            if (chunks_parsed_row != chunks_in_row) {
                // Number of pixels until new line in chunk
                uint8_t pixels_to_chunk = width * chunks_parsed_column + chunk_size * chunks_parsed_row;
                
                in_file.close();
                in_file.open(filename); // Re-open the file and start from the begining
    
                char trash[2];
                in_file.read(trash, 2); // Read the header of the ppm file (P3)
    
                int width, height, colours;
                in_file >> width >> height >> colours;
    
                // Navigate to new chunk line
                while (pixels_to_chunk > 0 && in_file >> R >> G >> B)
                {
                    pixels_to_chunk--;
                }

                std::cout << "Going to new chunk " << int(chunks_parsed_row) << std::endl;
            }
        }
        if (chunks_parsed_row >= chunks_in_row)
        {
            chunks_parsed_column += 1;
            chunks_parsed_row = 0;

            std::cout << "End of row " << int(chunks_parsed_column) << std::endl;
        }
    }

    
    std::string sprite_name;
    std::string const extension = ".ppm";

    // Getting the name of the sprite from the image name
    for (int i = filename.size() - extension.size() - 1; i >= 0; i--) {
        if (filename.at(i) == '/') {
            sprite_name = filename.substr(i + 1, filename.size() - extension.size() - 1 - i);
            break;
        }
    }

    std::ofstream tiles_stream("./parsing/" + sprite_name + ".tiles");
    std::ofstream palettes_stream("./parsing/" + sprite_name + ".palettes");

    // Writing the parsed data to files to be read by the game
    write_chunk("tile", tiles, &tiles_stream);
    write_chunk("palt", palettes, &palettes_stream);
}

int main()
{
    PPM_Parser parser;
    parser.parse_image("./sprites/bigger_ouchie.ppm", "parsing/tmp_chunk.txt");
}
