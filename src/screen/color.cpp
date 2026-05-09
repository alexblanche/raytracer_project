#include "screen/color.hpp"
#include <vector>

// Parallel for-loop macros
#include "parallel/parallel.h"
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})

#include <cmath>

namespace rt {

	const color color::WHITE(255, 255, 255);
	const color color::BLACK(0, 0, 0);
	const color color::BLUE(0, 0, 255);
	const color color::GREEN(0, 255, 0);
	const color color::RED(255, 0, 0);

	color color::max_out() const {
		const real maxed_red   = std::min(red,   static_cast<real>(255.0));
		const real maxed_green = std::min(green, static_cast<real>(255.0));
		const real maxed_blue  = std::min(blue,  static_cast<real>(255.0));
		return rt::color(maxed_red, maxed_green, maxed_blue);
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
