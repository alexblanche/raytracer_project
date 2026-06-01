#include "screen/color.hpp"
#include "parallel/parallel.hpp"
#include <vector>
#include <cmath>

namespace rt {

	[[nodiscard]] color color::max_out() const {
		constexpr real max = 255.0;
		return rt::color(
			std::min(red,   max),
			std::min(green, max),
			std::min(blue,  max)
		);
	}

	color add_col_vect(std::span<const color> color_set) {
		
		color col;
		for (color const& c : color_set)
			col += c;

		return col.max_out();
	}

	/* Returns the average of all the colors of the given color vector */
	color average_col_vect(std::span<const color> color_set) {
		const real n = color_set.size();
		color col;

		for (color const& c : color_set)
			col += c;

		return col / n;
	}
}
