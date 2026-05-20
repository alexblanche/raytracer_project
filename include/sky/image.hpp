#pragma once
#include <SDL2/SDL.h>
#include "sky/color.hpp"

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

