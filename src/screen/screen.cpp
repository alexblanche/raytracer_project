#include <string>
#include <iostream>
#include <signal.h>
#include <SDL2/SDL.h>

#include "headers/screen.hpp"

namespace rt {

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

	/**
	 * Main constructor, uses width and height.
	 */
	screen::screen(int width, int height) : image(width, height) {
		if(initialized == 0) {
			if(SDL_Init( SDL_INIT_VIDEO ) == -1) {
				std::cerr << "Cannot initialize SDL : "
					<< SDL_GetError() << std::endl;
				exit(-1);
			}
			signal(SIGINT, sigint_handler);
		};
		initialized += 1;
	}

	/**
	 * Destructor. Decrements the initialized counter.
	 */
	screen::~screen() {
		initialized--;
	}

	/**
	 * Flushes the buffer to the screen
	 */
	void screen::update() const {
		SDL_RenderPresent(renderer);
	}

	/**
	 * @brief wait indefinitely for the next quit event
	 * @return true if we get a quit event, or false if there was an error while waiting for the quit event
	 */
	bool screen::wait_quit_event() const {
		SDL_Event event;
		while(SDL_WaitEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					return true;
					break;
			}
		}
		return false;
	}
}
