#pragma once

#include "screen/color.hpp"
#include "screen/sdl.hpp"
#include "image/image.hpp"
#include "main_menu/runtime_parameters.hpp"

namespace rt {

	using enum tone_mapping_parameters::mode;

	class screen {

		private:
			sdl::window 	window;
			sdl::renderer 	renderer;
			sdl::rect 		srcrect;
			sdl::rect 		dstrect;
			sdl::texture	texture;
		
			matrix& mat;
			[[maybe_unused]] int width;
			[[maybe_unused]] int height;
			tone_mapping_parameters::mode tone_mapping_mode;
			float gamma;
			
		public:
			screen(matrix& matrix, tone_mapping_parameters::mode mode = Disabled, float gamma = 1.0f);
			screen(image& image,   tone_mapping_parameters::mode mode = Disabled);
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

			inline void copy_to_texture(const unsigned int number_of_rays) const {

				switch (tone_mapping_mode) {
					case Disabled: 	fast_copy(number_of_rays); 			 break;
					case Gamma: 	fast_copy_gamma(number_of_rays); 	 break;
					case Reinhardt:	fast_copy_reinhardt(number_of_rays); break;
				}
    		}
	};

}

