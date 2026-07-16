#pragma once

#include "auxiliary/exit_status.hpp"
#include "image/image.hpp"
#include "file_readers/error.hpp"

#include <expected>

class bmp {

	public:

		struct header {
			uint32_t file_size;
			unsigned char reserved1[2];
			unsigned char reserved2[2];
			uint32_t offset;
			uint32_t header_size;
			uint32_t width;
			uint32_t height;
			uint16_t color_planes;
			uint16_t bits_per_pixel;
			uint32_t compression_method;
			uint32_t compressed_size;
			uint32_t horizontal_resolution;
			uint32_t vertical_resolution;
			uint32_t colors_used;
			uint32_t important_colors;

			void print() const {
				printf("File size:             %u bytes\n", file_size                 );
				printf("Reserved 1:            0x%x%x\n",   reserved1[0], reserved1[1]);
				printf("Reserved 2:            0x%x%x\n",   reserved2[0], reserved2[1]);
				printf("Offset:                %u\n",       offset                    );
				printf("Header size:           %u bytes\n", header_size               );
				printf("Width:                 %u\n",       width                     );
				printf("Height:                %u\n",       height                    );
				printf("Color planes:          %u\n",       color_planes              );
				printf("Bits per pixel:        %u\n",       bits_per_pixel            );
				printf("Compression method:    %u\n",       compression_method        );
				printf("Compressed size:       %u bytes\n", compressed_size           );
				printf("Horizontal resolution: %u\n",       horizontal_resolution     );
				printf("Vertical resolution:   %u\n",       vertical_resolution       );
				printf("Colors used:           %u\n",       colors_used               );
				printf("Important colors:      %u\n",       important_colors          );
			}
		};

		/* Extracts the data from the given .bmp file: stores the width and height in the provided
			references, and returns a matrix of width rows and height columns containing colors
			Returns true if the operation was successful */
		static std::expected<matrix, file_reader::error> read_file(const std::string& file_name);

		/* Prints the info contained in the header of the given .bmp file */
		static exit_status print_info(const std::string& file_name);

		/* Writes the data into a .bmp file with the given name
			The value (real) of each component of each color of data is divided by number_of_rays before being written in the file
			Returns true if the operation was successful */
		static exit_status export_data(const std::string& file_name, const image& image);
};