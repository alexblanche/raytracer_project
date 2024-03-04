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

#include <SDL2/SDL.h>

namespace rt {

	/**
	 * The rect class just renames the SDL_Rect structure.
	 * It provides signed short x and y for the upper-left corner's coordinates,
	 * and unsigned short w and h for width and height.
	 */
	class rect : public SDL_Rect {

		public :

			/**
			 * Main constructor
			 */
			rect(int x, int y, int w, int h) {
				x = x;
				y = y;
				w = w;
				h = h;
			}

	};

}

