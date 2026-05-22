#pragma once

#include "screen/color.hpp"
#include "screen/sdl.hpp"

namespace rt {

	class screen {

		public:
			SDL_Window* window;
			SDL_Renderer* renderer;
			SDL_Rect srcrect;
    		SDL_Rect dstrect;
			SDL_Texture* texture;

			screen(int width, int height);
			~screen();

			void set_pixel(int x, int y, const color& c) const;
			void set_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) const;
			void clear() const;
			void update() const;
			void update_from_texture() const;

			/****************************************************************************************************/
			/** Event processing **/

			enum class quit_event {
				QuitEvent, KeyBoardEvent, Error	
			};

			/* Wait indefinitely for the next quit event */
			quit_event wait_quit_event() const;

			/* Stop at the next quit event */
			quit_event is_quit_event() const;

			enum class key {
				QuitEvent, SpaceEnter, B, R, Other
			};

			/* Wait indefinitely for the next keyboard or quit event */
			key wait_keyboard_event() const;

			/* Same as wait_keyboard_event, with poll events */
			key poll_keyboard_event() const;

			/****************************************************************************************************/

			/* Copies the rt::color matrix onto the screen, by averaging the number_of_rays colors per pixel */
			void fast_copy(std::vector<std::vector<rt::color>>& matrix,
				size_t width, size_t height,
				unsigned int number_of_rays) const;

			void fast_copy_gamma(std::vector<std::vector<rt::color>>& matrix,
				size_t width, size_t height,
				unsigned int number_of_rays, real gamma) const;

			void fast_copy_reinhardt(std::vector<std::vector<rt::color>>& matrix,
				size_t width, size_t height,
				unsigned int number_of_rays, real gamma) const;
	};

}

