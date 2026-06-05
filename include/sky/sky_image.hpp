#pragma once

#include "sky/sky_color.hpp"

#include <SDL2/SDL.h>

namespace sky {

	class image	{
		
		public:
			SDL_Window   * window;
			SDL_Renderer * renderer;

			SDL_Rect srcrect;
    		SDL_Rect dstrect;
			//SDL_Texture* texture;

			image(int width, int height);
			~image();
	};
}

