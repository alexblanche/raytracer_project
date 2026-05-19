#include <string>
#include <iostream>
#include <signal.h>
#include <SDL2/SDL.h>
#include <cmath>

#include "sky/screen.hpp"

namespace sky {

	/**
	 * The screen class inherites from the image class in order
	 * to draw something on the screen. It also wraps the SDL
	 * initialization calls. Only one screen should be created.
	 */

	// Indicates how many instances of screen exist.
	int screen::initialized = 0;

	static void sigint_handler(int) {
		exit(0);
	}

	screen::screen(int width, int height)
		
		: image(width, height) {

		if(initialized == 0) {
			if(SDL_Init(SDL_INIT_VIDEO) == -1) {
				std::cerr << "Cannot initialize SDL : "
					<< SDL_GetError() << std::endl;
				exit(-1);
			}
			signal(SIGINT, sigint_handler);
		}
		initialized += 1;
	}

	screen::~screen() {
		initialized--;
		//printf("Destroyed screen\n");
	}
}
