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

		vector(const real& a, const real& b, const real& c) : x(a), y(b), z(c) {}

		/**
		 * Comparison
		 */
		bool operator==(const vector& other) const;

		/**
		 * Addition: (a,b,c) + (d,e,f) = (a+d,b+e,c+f)
		 */
		vector operator+(const vector& other) const;

		/**
		 * Substraction (a,b,c) - (d,e,f) = (a-d,b-e,c-f)
		 */
		vector operator-(const vector& other) const;

		/**
		 * Vectorial product 
		 * (a,b,c) ^ (d,e,f) = (bf-ce,cd-af,ae-bd)
		 */
		vector operator^(const vector& other) const;

		/**
		 * Scalar product
		 * ((a,b,c) | (d,e,f)) = ad + be + cf
		 */
		real operator|(const vector& other) const;

		/**
		 * Returns the norm of the vector
		 */
		real norm() const;

		/**
		 * Returns the squared norm of the vector (x^2+y^2+z^2)
		 */
		real normsq() const;

		/**
		 * return a vector of the same direction but of norm 1
		 */
		vector unit() const;
	};

	/**
	 * Left multiplication with a scalar
	 * x * (a,b,c) = (xa,xb,xc)
	 */
	vector operator*(const real& x, const vector& v);

	/**
	 * Right multiplication with a scalar
	 * (a,b,c) * x = (ax,bx,cx)
	 */
	vector operator*(const vector& v, const real& x);

	/**
	 * Division by a scalar
	 * (a,b,c) / x = (a/x, b/x, c/x)
	 */
	vector operator/(const vector& v, const real& a);
}


