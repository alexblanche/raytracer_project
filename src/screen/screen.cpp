#include <string>
#include <iostream>
#include <signal.h>
#include <SDL2/SDL.h>

#include "headers/screen.hpp"

namespace rt {

	int screen::initialized = 0;

	static void sigint_handler(int) {
		exit(0);
	}

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

	screen::~screen() {};

	void screen::update() {
		SDL_RenderPresent(renderer);
	}

	bool screen::wait_quit_event() {
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
