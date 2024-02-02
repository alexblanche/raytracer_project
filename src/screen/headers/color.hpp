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

#include <vector>

namespace rt {

	/**
	 * The color class represents a color through its
	 * four components: red, green, blue and alpha (RGBA).
	 */
	class color {
		private:
			unsigned char red, green, blue, alpha; /*!< Red, green blue and alpha components */

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
			color(unsigned char r, unsigned char g, unsigned char b);

			/**
			 * Builds a color from its red, green, blue and alpha components.
			 */
			color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

			/**
			 * Sets the red component.
			 */
			virtual inline void set_red(unsigned char r) {
				red = r;
			}

			/**
			 * Returns the red component.
			 */
			virtual inline unsigned char get_red() const {
				return red;
			}

			/**
			 * Sets the green component.
			 */
			virtual inline void set_green(unsigned char g) {
				green = g;
			}

			/**
			 * Returns the green component.
			 */
			virtual inline unsigned char get_green() const {
				return green;
			}

			/**
			 * Sets the blue component.
			 */
			virtual inline void set_blue(unsigned char b) {
				blue = b;
			}

			/**
			 * Returns the blue component.
			 */
			virtual inline unsigned char get_blue() const {
				return blue;
			}

			/**
			 * Sets the alpha component.
			 */
			virtual inline void set_alpha(unsigned char a) {
				alpha = a;
			}

			/**
			 * Returns the alpha component.
			 */
			virtual inline unsigned char get_alpha() const {
				return alpha;
			}

			/**
			 * Comparison operator.
			 */
			virtual bool operator==(const color& c) const;

			/**
			 * Cast operator to int.
			 * Represents the color as a int, with
			 * red being the most significant byte.
			*/
			virtual operator int() const;

			/**
			 * Scaling operator.
			 */
			virtual color operator*(const double x) const;

			/**
			 * Addition operator.
			 */
			virtual color operator+(const color& c) const;

			/**
	 		* Product operator.
	 		*/
			virtual color operator*(const color& c) const;
	};

	/* Adds all the colors of the given color vector */
	color add_col_vect(const std::vector<color>& color_set);
}

