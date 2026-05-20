#pragma once
#include <SDL2/SDL.h>
#include "screen/color.hpp"


namespace rt {

	class image	{
		
		public:
			SDL_Window* window;
			SDL_Renderer* renderer;

			SDL_Rect srcrect;
    		SDL_Rect dstrect;
			SDL_Texture* texture;

			image();
			image(int width, int height);

			virtual ~image();

			virtual int width() const {
				int w, h;
				SDL_RenderGetLogicalSize(renderer, &w, &h);
				return w;
			}

			virtual int height() const {
				int w, h;
				SDL_RenderGetLogicalSize(renderer, &w, &h);
				return h;
			}

			virtual void set_pixel(int x, int y, const color& c) const;
			virtual void set_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) const;

			virtual void draw_line(int x1, int y1, int x2, int y2, const color& c);

			virtual void draw_rect(int x1, int y1, int x2, int y2, const color& c);

			virtual void fill_rect(int x1, int y1, int x2, int y2, const color& c);

			virtual void clear() const;
	};

}

