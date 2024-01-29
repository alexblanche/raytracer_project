#include "headers/color.hpp"

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
	color::color() : red(255), green(255), blue(255), alpha(255) {};

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
	color::color(unsigned char r, unsigned char g, unsigned char b) {
		red   = r;
		green = g;
		blue  = b;
		alpha = 255;
	}

	/**
	 * Builds a color from its red, green, blue and alpha components.
	 */
	color::color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
		red   = r;
		green = g;
		blue  = b;
		alpha = a;
	}

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
}
