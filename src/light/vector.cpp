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
#include "headers/vector.hpp"

namespace rt {

	/**
	 * This structure describes a 3D vector, providing
	 * basic operations (addition, scalar product, etc.)
	 * by overloading common operators.
	 */

	/**
	 * Comparison
	 */
	bool vector::operator==(const vector& other) const {
		return (other.x == x && other.y == y && other.z == z);
	}
	
	/**
	 * Addition: (a,b,c) + (d,e,f) = (a+d,b+e,c+f)
	 */
	vector vector::operator+(const vector& other) const {
		return vector(x+other.x,y+other.y,z+other.z);
	}

	/**
	 * Substraction (a,b,c) - (d,e,f) = (a-d,b-e,c-f)
	 */
	vector vector::operator-(const vector& other) const {
		return vector(x-other.x,y-other.y,z-other.z);
	}

	/**
	 * Vectorial product 
	 * (a,b,c) ^ (d,e,f) = (bf-ce,cd-af,ae-bd)
	 */
	vector vector::operator^(const vector& other) const {
		return
			vector(y*other.z - z*other.y,
				z*other.x - x*other.z,
				x*other.y - y*other.x
			);
	}

	/**
	 * Scalar product
	 * ((a,b,c) | (d,e,f)) = ad + be + cf
	 */
	double vector::operator|(const vector& other) const {
		return (x*other.x + y*other.y + z*other.z);
	}

	/**
	 * Returns the norm of the vector
	 */
	double vector::norm() const {
		double n = std::sqrt(x*x+y*y+z*z);
		return n;
	}

	/**
	 * return a vector of the same direction but of norm 1
	 */
	vector vector::unit() const {
		double n = norm();
		vector v(x/n,y/n,z/n);
		return v;
	}

	/**
	 * Left multiplication with a scalar
	 * x * (a,b,c) = (xa,xb,xc)
	 */
	vector operator*(const double& a, const vector& v) {
		return vector(a*v.x,a*v.y,a*v.z);
	}

	/**
	 * Right multiplication with a scalar
	 * (a,b,c) * x = (ax,bx,cx)
	 */
	vector operator*(const vector& v, const double& a) {
		return vector(a*v.x,a*v.y,a*v.z);
	}
}
