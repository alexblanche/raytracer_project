#include "other/sky/sky_screen.hpp"

#include <iostream>
#include <csignal>
#include <cmath>

namespace sky {

	screen::screen(int width, int height)
		
		: image(width, height) {

		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			std::cerr << "Cannot initialize SDL : "	<< SDL_GetError() << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}
