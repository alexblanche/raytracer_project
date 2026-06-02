#include <cmath>
#include "light/vector.hpp"

namespace rt {

	/* Rotation around axis x by an angle theta */
	vector vector::rotate_x(const real theta) const {
		const real costheta = cos(theta);
		const real sintheta = sin(theta);
		return vector(
			x,
			y * costheta - z * sintheta,
			y * sintheta + z * costheta
		);
	}

	/* Rotation around axis y by an angle theta */
	vector vector::rotate_y(const real theta) const {
		const real costheta = cos(theta);
		const real sintheta = sin(theta);
		return vector(
			x * costheta + z * sintheta,
			y,
			-x * sintheta + z * costheta
		);
	}

	/* Rotation around axis z by an angle theta */
	vector vector::rotate_z(const real theta) const {
		const real costheta = cos(theta);
		const real sintheta = sin(theta);
		return vector(
			x * costheta - y * sintheta,
			x * sintheta + y * costheta,
			z
		);
	}	
}
