/**
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmath>
#include "light/vector.hpp"

#include <cstdio>

namespace rt {

	/**
	 * This structure describes a 3D vector, providing
	 * basic operations (addition, scalar product, etc.)
	 * by overloading common operators.
	 */

	

	/**
	 * return a vector of the same direction but of norm 1
	 */
	vector vector::unit() const {
		const real n = norm();
		return vector(x/n, y/n, z/n);
	}

	/**
	 * Rotation around axis x by an angle theta
	 * */
	vector vector::rotate_x(const real theta) const {
		const real costheta = cos(theta);
		const real sintheta = sin(theta);
		return rt::vector(
			x,
			y*costheta - z*sintheta,
			y*sintheta + z*costheta
		);
	}

	/**
	 * Rotation around axis y by an angle theta
	 * */
	vector vector::rotate_y(const real theta) const {
		const real costheta = cos(theta);
		const real sintheta = sin(theta);
		return rt::vector(
			x*costheta + z*sintheta,
			y,
			-x*sintheta + z*costheta
		);
	}

	/**
	 * Rotation around axis z by an angle theta
	 * */
	vector vector::rotate_z(const real theta) const {
		const real costheta = cos(theta);
		const real sintheta = sin(theta);
		return rt::vector(
			x*costheta - y*sintheta,
			x*sintheta + y*costheta,
			z
		);
	}

	
}
