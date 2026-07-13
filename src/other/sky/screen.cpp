#include "other/sky/sky_screen.hpp"

#include <iostream>
#include <csignal>
#include <cmath>

namespace sky {

	enum VSync {
		Disable = 0, Enable = 1
	};

	screen::screen(int width, int height) {

		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			std::cerr << "Cannot initialize SDL : "	<< SDL_GetError() << std::endl;
			exit(EXIT_FAILURE);
		}

		window = SDL_CreateWindow("Sky_test", 10, 10, width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
		renderer = SDL_CreateRenderer(window, (-1), SDL_RENDERER_ACCELERATED);
		SDL_RenderSetVSync(renderer, VSync::Enable);
		SDL_RenderSetLogicalSize(renderer, width, height);
	}

	screen::~screen() noexcept {

		if (renderer != nullptr)
			SDL_DestroyRenderer(renderer);
		
		if(window != nullptr)
			SDL_DestroyWindow(window);
		
		SDL_Quit();
	}
}
