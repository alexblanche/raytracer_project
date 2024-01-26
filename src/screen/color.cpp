#include "headers/color.hpp"

namespace rt {

	color color::WHITE = color(255,255,255);
	color color::BLACK = color(0,0,0);
	color color::BLUE  = color(0,0,255);
	color color::GREEN = color(0,255,0);
	color color::RED   = color(255,0,0);

	color::color() : red(255), green(255), blue(255), alpha(255) {};

	color::color(const color& c) {
		red   = c.get_red();
		green = c.get_green();
		blue  = c.get_blue();
		alpha = c.get_alpha();
	}

	color::color(unsigned char r, unsigned char g, unsigned char b) {
		red   = r;
		green = g;
		blue  = b;
		alpha = 255;
	}

	color::color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
		red   = r;
		green = g;
		blue  = b;
		alpha = a;
	}

	bool color::operator==(const color& c) const {
		return (c.get_red()   == get_red())
			&& (c.get_green() == get_green())
			&& (c.get_blue()  == get_blue())
			&& (c.get_alpha() == get_alpha());
	}

	color::operator int() const {
		return 256*(256*(256*get_red() + get_green()) + get_blue()) + get_alpha();
	}
}
