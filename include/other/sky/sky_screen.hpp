#pragma once

#include <SDL2/SDL.h>

namespace sky {

	class screen {

		public:
			SDL_Window* window;
			SDL_Renderer* renderer;

			SDL_Rect srcrect;
			SDL_Rect dstrect;

			screen(int width, int height);

			~screen() noexcept;
	};

}

