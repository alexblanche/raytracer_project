#include "headers/color.hpp"
#include <vector>


namespace rt {

	/**
	 * The color class represents a color through its
	 * four components: red, green, blue and alpha (RGBA).
	 */

	color color::WHITE = color(255,255,255);
	color color::BLACK = color(0,0,0);
	color color::BLUE  = color(0,0,255);
	color color::GREEN = color(0,255,0);
	color color::RED   = color(255,0,0);

	/**
	 * Default constructor. Builds a white color.
	 */
	color::color() : red(255), green(255), blue(255), alpha(255) {}

	/**
	 * Copy constructor.
	 */
	color::color(const color& c) {
		red   = c.get_red();
		green = c.get_green();
		blue  = c.get_blue();
		alpha = c.get_alpha();
	}

	/**
	 * Builds a color from its red, green and blue components.
	 * Alpha is set to 255.
	 */
	color::color(unsigned char r, unsigned char g, unsigned char b)
		: red(r), green(g), blue(b), alpha(255) {}


	/**
	 * Builds a color from its red, green, blue and alpha components.
	 */
	color::color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
		: red(r), green(g), blue(b), alpha(a) {}

	/**
	 * Comparison operator.
	 */
	bool color::operator==(const color& c) const {
		return (c.get_red()   == get_red())
			&& (c.get_green() == get_green())
			&& (c.get_blue()  == get_blue())
			&& (c.get_alpha() == get_alpha());
	}

	/**
	 * Cast operator to int.
	 * Represents the color as a int, with
	 * red being the most significant byte.
	*/
	color::operator int() const {
		return 256*(256*(256*get_red() + get_green()) + get_blue()) + get_alpha();
	}

	/* Adds all the colors of the given color vector */
	color add_col_vect(const std::vector<color>& color_set) {
		const unsigned int n = color_set.size();
		int r = 0;
		int g = 0;
		int b = 0;
		const rt::color *c;

		for (unsigned int i = 0; i < n; i++) {
			c = &(color_set.at(i));

			r += c->get_red();
			g += c->get_green();
			b += c->get_blue();
		}

		if (r > 255) {r = 255;}
		if (g > 255) {g = 255;}
		if (b > 255) {b = 255;}

		return rt::color(r, g, b);
	}
}
