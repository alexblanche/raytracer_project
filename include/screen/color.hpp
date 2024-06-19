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

// Type alias for floating-point numerical values
using real = float;

#include <vector>

namespace rt  {

	/**
	 * The color class represents a color through its
	 * three components: red, green, blue (RGB).
	 */
	class color {
		private:
			/* Red, green blue and alpha components */
			real red, green, blue;

		public:

			/**
			 * Preset colors
			*/
			static const color WHITE;
			static const color BLACK;
			static const color BLUE;
			static const color GREEN;
			static const color RED;

			/**
			 * Default constructor. Builds a white color.
			 */
			color();

			/**
			 * Copy constructor.
			 */
			color(const color& c);

			/**
			 * Builds a color from its red, green and blue components.
			 * Alpha is set to 255.
			 */
			color(const real& r, const real& g, const real& b);

			/**
			 * Assignment by copy
			 */
			void operator=(const color& c);

			/**
			 * Sets the red component.
			 */
			inline void set_red(const real& r) {
				red = r;
			}

			/**
			 * Returns the red component.
			 */
			inline real get_red() const {
				return red;
			}

			/**
			 * Sets the green component.
			 */
			inline void set_green(const real& g) {
				green = g;
			}

			/**
			 * Returns the green component.
			 */
			inline real get_green() const {
				return green;
			}

			/**
			 * Sets the blue component.
			 */
			inline void set_blue(const real& b) {
				blue = b;
			}

			/**
			 * Returns the blue component.
			 */
			inline real get_blue() const {
				return blue;
			}

			/**
			 * Comparison operator.
			 */
			bool operator==(const color& c) const;

			/**
			 * Scaling operator.
			 */
			color operator*(const real& x) const;

			/**
			 * Addition operator.
			 */
			color operator+(const color& c) const;

			/**
	 		* Product operator.
	 		*/
			color operator*(const color& c) const;

			/**
			 * Division operator.
			 */
			color operator/(const real& x) const;

			/**
			 * Maxing out color components at 255.
			 */
			color max_out() const;
	};

	/* Adds all the colors of the given color vector */
	color add_col_vect(const std::vector<color>& color_set);

	/* Returns the average of all the colors of the given color vector */
	color average_col_vect(const std::vector<color>& color_set);
}

