/**
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <SDL2/SDL.h>

#include "image.hpp"

namespace rt {

	/**
	 * The screen class inherites from the image class in order
	 * to draw something on the screen. It also wraps the SDL
	 * initialization calls. Only one screen should be created.
	 */
	class screen : public image	{
		
		private:
			static int initialized; /*!< Indicates how many instances of screen exist. */

		public:
			/**
			 * Main constructor, uses width and height.
			 */
			screen(int width, int height);

			/**
			 * Destructor. Decrements the initialized
			 * counter and closes the screen only if it goes to 0.
			 */
			~screen();

			/**
			 * Flushes the buffer to the screen
			 */
			void update() const;

			/****************************************************************************************************/
			/** Event processing **/

			/**
			 * @brief Wait indefinitely for the next quit event
			 * @return true if we get a quit event, or false if there was an error while waiting for the quit event
			 */
			bool wait_quit_event() const;

			/**
			 * @brief Stop at the next quit event
			 * @return true if we get a quit event, false if we get a keydown event
			 */
			bool is_quit_event() const;

			/**
			 * @brief Wait indefinitely for the next keyboard or quit event
			 * @return
			 * 		1: quit event (Esc or X clicked)
			 * 		2: Space or Enter key
			 * 		3: 'B' key
			 * 		4: 'R' key
			 * 		0: Anything else
			 */
			int wait_keyboard_event() const;

			/****************************************************************************************************/

			/**
			 * Copies the rt::color matrix onto the screen, by averaging the number_of_rays colors per pixel
			*/
			void copy(std::vector<std::vector<rt::color>>& matrix,
				const size_t width, const size_t height,
				const unsigned int number_of_rays) const;

			/**
			 * Copies the rt::color matrix onto the screen, by averaging the number_of_rays colors per pixel
			 * and applying a square root to each component to increase the brightness
			*/
			void copy_gamma_corrected(std::vector<std::vector<rt::color>>& matrix,
				const size_t width, const size_t height,
				const unsigned int number_of_rays) const;
	};

}

