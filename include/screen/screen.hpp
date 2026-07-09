#pragma once

#include "screen/color.hpp"
#include "screen/sdl.hpp"
#include "image/image.hpp"
#include "main_menu/runtime_parameters.hpp"

namespace rt {

	class screen {

		using enum tone_mapping_parameters::mode;

		private:
			sdl::window 	window;
			sdl::renderer 	renderer;
			sdl::rect 		srcrect;
			sdl::rect 		dstrect;
			sdl::texture	texture;
		
			image& img;
			tone_mapping_parameters::mode tone_mapping_mode;
			
		public:
			screen(image& image, tone_mapping_parameters::mode mode = Disabled);
			
			screen(screen&&) 		          = delete;
			screen(const screen&)             = delete;
			screen& operator=(const screen&)  = delete;
			screen& operator=(screen&&)       = delete;

			~screen() noexcept;

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
			void fast_copy(unsigned int number_of_rays) const;

			void fast_copy_gamma(unsigned int number_of_rays) const;

			void fast_copy_reinhardt(unsigned int number_of_rays) const;

			inline void copy_to_texture(unsigned int number_of_rays = 0) const {

				number_of_rays = (number_of_rays == 0) ? img.number_of_samples : number_of_rays;

				switch (tone_mapping_mode) {
					case Disabled: 	fast_copy(number_of_rays); 			 break;
					case Gamma: 	fast_copy_gamma(number_of_rays); 	 break;
					case Reinhardt:	fast_copy_reinhardt(number_of_rays); break;
				}
    		}

			inline void refresh() const {
				copy_to_texture(img.number_of_samples);
        		update_from_texture();
			}
	};

}

