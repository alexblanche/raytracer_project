#include "other/sky/sky_image.hpp"

#include <iostream>

namespace sky {

	enum VSync {
		Disable = 0, Enable = 1
	};

	image::image(int width, int height)	{
		//SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE, &window, &renderer);
		window = SDL_CreateWindow("Sky_test", 10, 10, width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
		renderer = SDL_CreateRenderer(window, (-1), SDL_RENDERER_ACCELERATED);
		SDL_RenderSetVSync(renderer, VSync::Disable);
		SDL_RenderSetLogicalSize(renderer, width, height);

		//texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
	}

	image::~image() {
		//SDL_DestroyTexture(texture);
		if (renderer != nullptr)
			SDL_DestroyRenderer(renderer);
		
		if(window != nullptr)
			SDL_DestroyWindow(window);
		
		SDL_Quit();
	}
}
