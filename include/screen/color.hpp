#pragma once

#include "parameters.hpp"

#include <vector>
#include <span>
#include <cmath>

namespace rt  {

	struct color {

		real red, green, blue;

		constexpr color()
			: red(0.0_r), green(0.0_r), blue(0.0_r) {}

		constexpr color(const real r, const real g, const real b)
			: red(r), green(g), blue(b) {}

		constexpr color(const color&) 	 = default;
		constexpr color(color&&) 		 = default;
		color& operator=(const color& c) = default;
		color& operator=(color&&)		 = default;


		inline real get_average() const {
			return (red + green + blue) * (1.0_r / 3.0_r);
		}

		// Same between 0 and 1
		inline real get_average_ratio() const {
			return (red + green + blue) * (1.0_r / (3.0_r * 255.0_r));
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
			constexpr real inv255 = 1.0_r / 255.0_r;
			return color(
				red   * c.red   * inv255,
				green * c.green * inv255,
				blue  * c.blue  * inv255
			);
		}
		
		inline color operator/(const real x) const {
			const real invx = 1.0_r / x;
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
			constexpr real inv255 = 1.0_r / 255.0_r;
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

		// exponentiation operator
		inline void operator^=(const real a) {
			red   = pow(red,   a);
			green = pow(green, a);
			blue  = pow(blue,  a);
		}

		inline void apply_gamma(const real gamma) {
			(*this) /= 255.0_r;
			(*this) ^= gamma;
			(*this) *= 255.0_r;
		}

		/* Maxing out color components at 255. */
		color max_out() const;

		inline void in_place_max_out() {
			constexpr real max = 255.0;
			red   = std::min(red,   max);
			green = std::min(green, max);
			blue  = std::min(blue,  max);
		}

		struct uint8_color {
			uint8_t r, g, b;
		};

		inline uint8_color to_uint8() const {
			return {
				static_cast<uint8_t>(red),
				static_cast<uint8_t>(green),
				static_cast<uint8_t>(blue)
			};
		}

		inline uint8_color to_uint8_bgr() const {
			return {
				static_cast<uint8_t>(blue),
				static_cast<uint8_t>(green),
				static_cast<uint8_t>(red)
			};
		}
	};

	constexpr color WHITE(255, 255, 255);
	constexpr color BLACK(0,   0,   0  );
	constexpr color BLUE (0,   0,   255);
	constexpr color GREEN(0,   255, 0  );
	constexpr color RED  (255, 0,   0  );
	

	/* Adds all the colors of the given color vector */
	color add_col_vect(std::span<const color> color_set);

	/* Returns the average of all the colors of the given color vector */
	color average_col_vect(std::span<const color> color_set);

	// Returns c1 * a + c2
	inline color fma(const color& c1, const real a, const color& c2) {
		return
			color(
				std::fma(c1.red,   a, c2.red),
				std::fma(c1.green, a, c2.green),
				std::fma(c1.blue,  a, c2.blue)
			);
	}

	// Returns c1 * c2 + c3
	inline color fma(const color& c1, const color& c2, const color& c3) {

		return
			color(
				std::fma(c1.red,   c2.red   / 255.0_r, c3.red),
				std::fma(c1.green, c2.green / 255.0_r, c3.green),
				std::fma(c1.blue,  c2.blue  / 255.0_r, c3.blue)
			);
	}
}

