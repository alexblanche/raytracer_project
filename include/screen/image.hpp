#pragma once
#include <SDL2/SDL.h>
#include "screen/color.hpp"


namespace rt {

	/**
	 * The image class wraps a window and a SDL renderer
	 * to offer object-oriented access to its data.
	 * It represents an in-memory image.
	 */
	class image	{
		
		public:
		// protected:
			
			SDL_Window* window;
			SDL_Renderer* renderer;

			SDL_Rect srcrect;
    		SDL_Rect dstrect;
			SDL_Texture* texture;

			/**
			 * Default constructor is protected and can be used
			 * only by inheriting classes. It is forbidden to
			 * build an image without at least its dimensions.
			 */
			image();

		// public:

			/**
			 * Main constructor. Builds an image from its dimensions.
			 */
			image(int width, int height);

			/**
			 * Copy constructor.
			 * Warning: this does not copy the actual data, so
			 * any modification to the copy will impact the original.
			 * To build a hard copy, use the copy member function.
			 
			image(const image& img);
			*/

			/**
			 * Destructor. Will decrease the reference counter
			 * of the wrapped data and free it if it goes to 0.
			 */
			virtual ~image();

			/**
			 * Returns the width of the image.
			 */
			virtual int width() const {
				int w, h;
				SDL_RenderGetLogicalSize(renderer, &w, &h);
				return w;
			}

			/**
			 * Returns the height of the image.
			 */
			virtual int height() const {
				int w, h;
				SDL_RenderGetLogicalSize(renderer, &w, &h);
				return h;
			}

			/**
			 * Sets a pixel to a given color.
			 */
			virtual void set_pixel(int x, int y, const color& c) const;
			virtual void set_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) const;

			/**
			 * Draws a line from (x1,y1) to (x2,y2) of a given color.
			 */
			virtual void draw_line(int x1, int y1, int x2, int y2, const color& c);

			/**
			 * Draws a rectangle (uses draw_line) of a given color.
			 */
			virtual void draw_rect(int x1, int y1, int x2, int y2, const color& c);

			/**
			 * Draws a filled rectangle of a given color.
			 */
			virtual void fill_rect(int x1, int y1, int x2, int y2, const color& c);

			/**
			 * Erases the content of the buffer
			 */
			virtual void clear() const;
	};

}

