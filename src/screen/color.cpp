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

	color add_col_vect(const std::vector<color>& color_set) {
		
		color col;
		for (rt::color const& c : color_set)
			col += c;

		return col.max_out();
	}

	/* Returns the average of all the colors of the given color vector */
	color average_col_vect(const std::vector<color>& color_set) {
		const real n = color_set.size();
		color col;

		for (rt::color const& c : color_set)
			col += c;

		return col / n;
	}

	/* Applies gamma correction to the color data */
	void apply_gamma(std::vector<std::vector<color>>& data, const real gamma) {

		const unsigned int width = data.size();
		const unsigned int height = data[0].size();

		PARALLEL_FOR_BEGIN(width) {

			std::vector<color>& data_line = data[i];
			for (unsigned int j = 0; j < height; j++) {
				const color& col = data_line[j];
				const real r = pow(col.get_red()   * (1.0f / 255.0f), gamma) * 255.0f;
				const real g = pow(col.get_green() * (1.0f / 255.0f), gamma) * 255.0f;
				const real b = pow(col.get_blue()  * (1.0f / 255.0f), gamma) * 255.0f;
				data_line[j] = rt::color(r, g, b);
			}
		} PARALLEL_FOR_END();
	}
}
