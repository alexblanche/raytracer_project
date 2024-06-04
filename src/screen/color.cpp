#include "screen/color.hpp"
#include <vector>


namespace rt {

	/**
	 * The color class represents a color through its
	 * four components: red, green, blue and alpha (RGBA).
	 */

	const color color::WHITE = color(255,255,255);
	const color color::BLACK = color(0,0,0);
	const color color::BLUE  = color(0,0,255);
	const color color::GREEN = color(0,255,0);
	const color color::RED   = color(255,0,0);

	/**
	 * Default constructor. Builds a black color.
	 */
	color::color() : red(0), green(0), blue(0) {}

	/**
	 * Copy constructor.
	 */
	color::color(const color& c) {
	 	red   = c.get_red();
	 	green = c.get_green();
	 	blue  = c.get_blue();
	}

	/**
	 * Builds a color from its red, green and blue components.
	 * Alpha is set to 255.
	 */
	color::color(const double& r, const double& g, const double& b)
		: red(r), green(g), blue(b) {}

	
	/**
	 * Assignment by copy
	 */
	void color::operator=(const color& c) {
		red = c.get_red();
		green = c.get_green();
		blue = c.get_blue();
	}

	/**
	 * Comparison operator.
	 */
	bool color::operator==(const color& c) const {
		return (c.get_red()   == get_red())
			&& (c.get_green() == get_green())
			&& (c.get_blue()  == get_blue());
	}

	/**
	 * Scaling operator.
	 */
	color color::operator*(const double x) const {
		return color(
			x * get_red(),
			x * get_green(),
			x * get_blue());
	}

	/**
	 * Addition operator.
	 */
	color color::operator+(const color& c) const {
		return color(
			get_red()   + c.get_red(),
			get_green() + c.get_green(),
			get_blue()  + c.get_blue());
	}

	/**
	 * Product operator.
	 */
	color color::operator*(const color& c) const {
		return color(
			get_red() 	* c.get_red() 	/ 255.0,
			get_green() * c.get_green() / 255.0,
			get_blue() 	* c.get_blue() 	/ 255.0);
	}

	/**
	 * Division by a scalar operator.
	 */
	color color::operator/(const double x) const {
		return color(
			get_red() 	/ x,
			get_green() / x,
			get_blue() 	/ x);
	}

	/* Adds all the colors of the given color vector */
	color add_col_vect(const std::vector<color>& color_set) {
		
		double r = 0;
		double g = 0;
		double b = 0;

		for (rt::color const& c : color_set) {

			r += c.get_red();
			g += c.get_green();
			b += c.get_blue();
		}

		if (r > 255) { r = 255; }
		if (g > 255) { g = 255; }
		if (b > 255) { b = 255; }

		return rt::color(r, g, b);
	}

	/* Returns the average of all the colors of the given color vector */
	color average_col_vect(const std::vector<color>& color_set) {
		const unsigned int n = color_set.size();
		double r = 0;
		double g = 0;
		double b = 0;

		for (rt::color const& c : color_set) {

			r += c.get_red();
			g += c.get_green();
			b += c.get_blue();
		}

		return rt::color(r / n, g / n, b / n);
	}

	/**
	 * Maxing out color components at 255.
	 */
	color color::max_out() const {
		const double maxed_red   = std::min(red,   255.0);
		const double maxed_green = std::min(green, 255.0);
		const double maxed_blue  = std::min(blue,  255.0);
		return rt::color(maxed_red, maxed_green, maxed_blue);
	}
}
