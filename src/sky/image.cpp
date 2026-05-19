#include "sky/image.hpp"

#include <iostream>

namespace sky {

	image::image() {}

	image::image(int width, int height)	{
		//SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE, &window, &renderer);
		window = SDL_CreateWindow("Sky_test", 10, 10, width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
		renderer = SDL_CreateRenderer(window, (-1), SDL_RENDERER_ACCELERATED);
		SDL_RenderSetVSync(renderer, 0); // 1 = Enable VSync, 0 = Disable
		SDL_RenderSetLogicalSize(renderer, width, height);

		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = width;
		srcrect.h = height;
		dstrect.x = 0;
		dstrect.y = 0;
		dstrect.w = width;
		dstrect.h = height;

		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
	}

	image::~image() {
		SDL_DestroyTexture(texture);
		if (renderer != nullptr){
			SDL_DestroyRenderer(renderer);
		}
		if(window != nullptr){
			SDL_DestroyWindow(window);
		}
		SDL_Quit();
		//printf("Destroyed image\n");
	}
}
