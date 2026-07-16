#include "screen/color.hpp"
#include <vector>

namespace rt {

	[[nodiscard]] color color::get_capped() const {
		constexpr real max = 255.0_r;
		return color(
			std::min(red,   max),
			std::min(green, max),
			std::min(blue,  max)
		);
	}

	color add_col_vect(const std::span<const color> color_set) {
		
		color col;
		for (const color& c : color_set)
			col += c;

		return col.get_capped();
	}

	/* Returns the average of all the colors of the given color vector */
	color average_col_vect(const std::span<const color> color_set) {
		const real n = color_set.size();
		color col;

		for (const color& c : color_set)
			col += c;

		return col / n;
	}
}
