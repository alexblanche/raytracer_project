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

#include "parameters.hpp"
#include <vector>

#include <cmath>

namespace rt  {

	struct color {
		private:
			/* Red, green blue and alpha components */
			real red, green, blue;

		public:

			static const color WHITE;
			static const color BLACK;
			static const color BLUE;
			static const color GREEN;
			static const color RED;

			constexpr color()
				: red(0), green(0), blue(0) {}

			constexpr color(const real r, const real g, const real b)
				: red(r), green(g), blue(b) {}

			constexpr color(const color&) 	= default;
			constexpr color(color&&) 		= default;

			inline void operator=(const color& c) {
				red   = c.red;
				green = c.green;
				blue  = c.blue;
			}

			inline void set_red(const real r) {
				red = r;
			}

			inline real get_red() const {
				return red;
			}

			inline void set_green(const real g) {
				green = g;
			}

			inline real get_green() const {
				return green;
			}

			inline void set_blue(const real b) {
				blue = b;
			}

			inline real get_blue() const {
				return blue;
			}

			inline real get_average() const {
				return (red + green + blue) * (1.0f / 3.0f);
			}

			// Same between 0 and 1
			inline real get_average_ratio() const {
				return (red + green + blue) * (1.0f / (3.0f * 255.0f));
			}

			inline bool operator==(const color& c) const {
				return (c.red   == red)
					&& (c.green == green)
					&& (c.blue  == blue);
			}

			inline color operator*(const real x) const {
				return color(
					x * red,
					x * green,
					x * blue
				);
			}

			inline color operator+(const color& c) const {
				return color(
					red   + c.red,
					green + c.green,
					blue  + c.blue
				);
			}

			inline color operator*(const color& c) const {
				constexpr real inv255 = 1.0f / 255.0f;
				return color(
					red   * c.red   * inv255,
					green * c.green * inv255,
					blue  * c.blue  * inv255
				);
			}
			
			inline color operator/(const real x) const {
				const real invx = 1.0f / x;
				return color(
					red   * invx,
					green * invx,
					blue  * invx
				);
			}

			// In-place transformations
			inline void operator +=(const color& other) {
				red   += other.red;
				green += other.green;
				blue  += other.blue;
			}

			inline void operator -=(const color& other) {
				red   -= other.red;
				green -= other.green;
				blue  -= other.blue;
			}

			inline void operator *=(const color& other) {
				constexpr real inv255 = 1.0f / 255.0f;
				red   *= other.red   * inv255;
				green *= other.green * inv255;
				blue  *= other.blue  * inv255;
			}

			inline void operator *=(const real a) {
				red   *= a;
				green *= a;
				blue  *= a;
			}

			inline void operator /=(const real a) {
				red   /= a;
				green /= a;
				blue  /= a;
			}

			/**
			 * Maxing out color components at 255.
			 */
			color max_out() const;
	};

	/* Adds all the colors of the given color vector */
	color add_col_vect(const std::vector<color>& color_set);

	/* Returns the average of all the colors of the given color vector */
	color average_col_vect(const std::vector<color>& color_set);

	/* Applies gamma correction to the color data */
	void apply_gamma(std::vector<std::vector<color>>& data, const real gamma);

	// Returns c1 * a + c2
	inline color fma(const color& c1, const real a, const color& c2) {
		return
			color(
				std::fma(c1.get_red(),   a, c2.get_red()),
				std::fma(c1.get_green(), a, c2.get_green()),
				std::fma(c1.get_blue(),  a, c2.get_blue())
			);
	}

	// Returns c1 * c2 + c3
	inline color fma(const color& c1, const color& c2, const color& c3) {
		return
			color(
				std::fma(c1.get_red(),   c2.get_red()   / ((real) 255.0f), c3.get_red()),
				std::fma(c1.get_green(), c2.get_green() / ((real) 255.0f), c3.get_green()),
				std::fma(c1.get_blue(),  c2.get_blue()  / ((real) 255.0f), c3.get_blue())
			);
	}
}

