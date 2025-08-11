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
#pragma once

#include "screen/color.hpp"
#include <cmath>

namespace rt {
	
	/**
	 * This structure describes a 3D vector, providing
	 * basic operations (addition, scalar product, etc.)
	 * by overloading common operators.
	 */
	struct vector {
		
		real x, y ,z;
	
		// Constructors
		vector() : x(0), y(0), z(0) {}
		
		vector(const real a, const real b, const real c) : x(a), y(b), z(c) {}

		/**
		 * Comparison
		 */
		inline bool operator==(const vector& other) const {
			return (other.x == x && other.y == y && other.z == z);
		}
		
		/**
		 * Addition: (a,b,c) + (d,e,f) = (a+d,b+e,c+f)
		 */
		inline vector operator+(const vector& other) const {
			return vector(x+other.x, y+other.y, z+other.z);
		}

		/**
		 * Substraction (a,b,c) - (d,e,f) = (a-d,b-e,c-f)
		 */
		inline vector operator-(const vector& other) const {
			return vector(x-other.x, y-other.y, z-other.z);
		}

		/**
		 * Vectorial product 
		 * (a,b,c) ^ (d,e,f) = (bf-ce,cd-af,ae-bd)
		 */
		inline vector operator^(const vector& other) const {
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
		inline real operator|(const vector& other) const {
			return (x*other.x + y*other.y + z*other.z);
		}

		/**
		 * Returns the norm of the vector
		 */
		inline real norm() const {
			return std::sqrt(x*x + y*y + z*z);
		}

		/**
		 * Returns the squared norm of the vector (x^2+y^2+z^2)
		 */
		inline real normsq() const {
			return x*x + y*y + z*z;
		}

		/**
		 * return a vector of the same direction but of norm 1
		 */
		vector unit() const;

		/**
		 * Rotation around axis x by an angle theta
		 * */
		vector rotate_x(const real theta) const;

		/**
		 * Rotation around axis y by an angle theta
		 * */
		vector rotate_y(const real theta) const;

		/**
		 * Rotation around axis z by an angle theta
		 * */
		vector rotate_z(const real theta) const;

		// In-place transformations	
		inline void to_unit() {
			const real n = norm();
			x /= n;
			y /= n;
			z /= n;
		}
	};

	/**
	 * Left multiplication with a scalar
	 * x * (a,b,c) = (xa,xb,xc)
	 */
	inline vector operator*(const real a, const vector& v) {
		return vector(a*v.x, a*v.y, a*v.z);
	}

	/**
	 * Right multiplication with a scalar
	 * (a,b,c) * x = (ax,bx,cx)
	 */
	inline vector operator*(const vector& v, const real a) {
		return vector(a*v.x, a*v.y, a*v.z);
	}

	/**
	 * Division by a scalar
	 * (a,b,c) / x = (a/x, b/x, c/x)
	 */
	inline vector operator/(const vector& v, const real a) {
		return vector(v.x / a, v.y / a, v.z / a);
	}

	// In-place transformations
	inline void operator +=(vector& v, const vector& other) {
		v.x += other.x;
		v.y += other.y;
		v.z += other.z;
	}

	inline void operator -=(vector& v, const vector& other) {
		v.x -= other.x;
		v.y -= other.y;
		v.z -= other.z;
	}

	inline void operator *=(vector& v, const real a) {
		v.x *= a;
		v.y *= a;
		v.z *= a;
	}

	// Returns v1 * a + v2, where a is a scalar

	inline vector fma(const vector& v1, const real a, const vector& v2) {
		return
			vector(
				std::fma(v1.x, a, v2.x),
				std::fma(v1.y, a, v2.y),
				std::fma(v1.z, a, v2.z)
			);
	}
}


