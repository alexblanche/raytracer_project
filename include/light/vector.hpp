#pragma once

#include "parameters.hpp"

#include <cmath>
#include <algorithm>

namespace rt {
	
	struct vector {
		
		real x, y ,z;
	
		constexpr vector() : x(0), y(0), z(0) {}
		constexpr vector(real a, real b, real c) : x(a), y(b), z(c) {}

		vector(vector&&)                 noexcept = default;
        vector(const vector&)            noexcept = default;
        vector& operator=(vector&&)      noexcept = default;
        vector& operator=(const vector&) noexcept = default;

		inline bool operator==(const vector& other) const {
			return (other.x == x && other.y == y && other.z == z);
		}
		
		inline vector operator+(const vector& other) const {
			return vector(x + other.x, y + other.y, z + other.z);
		}

		inline vector operator-(const vector& other) const {
			return vector(x - other.x, y - other.y, z - other.z);
		}

		/* Element-wise product */
		inline vector operator*(const vector& other) const {
			return vector(x * other.x, y * other.y, z * other.z);
		}

		/* Cross product */
		inline constexpr vector operator^(const vector& other) const {
			return
				vector(
					y * other.z - z * other.y,
					z * other.x - x * other.z,
					x * other.y - y * other.x
				);
		}

		/* Dot product */
		inline real operator|(const vector& other) const {
			return std::fma(x, other.x, std::fma(y, other.y, z * other.z));
		}

		/* Returns the squared norm of the vector */
		inline real normsq() const {
			return std::fma(x, x, std::fma(y, y, z * z));
		}

		/* Returns the norm of the vector */
		inline real norm() const {
			return std::sqrt(normsq());
		}

		/* Returns a vector of the same direction but of norm 1 */
		inline vector unit() const {
			const real invn = 1.0f / norm();
			return vector(x * invn, y * invn, z * invn);
		}

		/* Rotation around axis x by an angle theta */
		inline vector rotate_x(const real theta) const {
			const real costheta = cos(theta);
			const real sintheta = sin(theta);
			return vector(
				x,
				y * costheta - z * sintheta,
				y * sintheta + z * costheta
			);
		}

		inline vector rotate_x(const real costheta, const real sintheta) const {
			
			return vector(
				x,
				y * costheta - z * sintheta,
				y * sintheta + z * costheta
			);
		}

		/* Rotation around axis y by an angle theta */
		inline vector rotate_y(const real theta) const {
			const real costheta = cos(theta);
			const real sintheta = sin(theta);
			return vector(
				x * costheta + z * sintheta,
				y,
				-x * sintheta + z * costheta
			);
		}

		inline vector rotate_y(const real costheta, const real sintheta) const {

			return vector(
				x * costheta + z * sintheta,
				y,
				-x * sintheta + z * costheta
			);
		}

		/* Rotation around axis z by an angle theta */
		inline vector rotate_z(const real theta) const {
			const real costheta = cos(theta);
			const real sintheta = sin(theta);
			return vector(
				x * costheta - y * sintheta,
				x * sintheta + y * costheta,
				z
			);
		}

		inline vector rotate_z(const real costheta, const real sintheta) const {

			return vector(
				x * costheta - y * sintheta,
				x * sintheta + y * costheta,
				z
			);
		}

		/* In-place transformations	*/
		inline void to_unit() {
			const real invn = 1.0f / norm();
			x *= invn;
			y *= invn;
			z *= invn;
		}
	};

	/* Left multiplication with a scalar */
	inline vector operator*(real a, const vector& v) {
		return vector(
			a * v.x,
			a * v.y,
			a * v.z
		);
	}

	/* Right multiplication with a scalar */
	inline vector operator*(const vector& v, real a) {
		return a * v;
	}

	/* Division by a scalar */
	inline vector operator/(const vector& v, const real a) {
		const real inva = 1.0f / a;
		return v * inva;
	}

	/* In-place transformations */
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

	inline void operator /=(vector& v, const real a) {
		const real inv_a = 1.0_r / a;
		v.x *= inv_a;
		v.y *= inv_a;
		v.z *= inv_a;
	}

	/* Returns v1 * a + v2, where a is a scalar */
	inline vector fma(const vector& v1, const real a, const vector& v2) {
		return
			vector(
				std::fma(v1.x, a, v2.x),
				std::fma(v1.y, a, v2.y),
				std::fma(v1.z, a, v2.z)
			);
	}

	/* Returns a1 * v1 + a2 * v2 + a3 * v3 */
	inline vector matprod(
		const vector& v1, const real a1,
		const vector& v2, const real a2,
		const vector& v3, const real a3
	) {
		return
			fma(v1,  a1,
			fma(v2,  a2,
				v3 * a3));
	}

	/* Returns v.x * v1 + v.y * v2 + v.z * v3 */
	inline vector matprod(const vector& v1, const vector& v2, const vector& v3, const vector& v) {
		return matprod(v1, v.x, v2, v.y, v3, v.z);
	}

	inline vector min(const vector& v1, const vector& v2) {
		return vector(
			std::min(v1.x, v2.x),
			std::min(v1.y, v2.y),
			std::min(v1.z, v2.z)
		);
	}

	inline vector max(const vector& v1, const vector& v2) {
		return vector(
			std::max(v1.x, v2.x),
			std::max(v1.y, v2.y),
			std::max(v1.z, v2.z)
		);
	}

	inline std::pair<vector, vector> min_max(const vector& v1, const vector& v2) {
		const auto& [ min_x, max_x ] = (v1.x < v2.x) ? std::pair { v1.x, v2.x } : std::pair { v2.x, v1.x };
		const auto& [ min_y, max_y ] = (v1.y < v2.y) ? std::pair { v1.y, v2.y } : std::pair { v2.y, v1.y };
		const auto& [ min_z, max_z ] = (v1.z < v2.z) ? std::pair { v1.z, v2.z } : std::pair { v2.z, v1.z };

		return { vector(min_x, min_y, min_z), vector(max_x, max_y, max_z) };
	}

	inline vector abs(const vector& v) {
		return vector(std::abs(v.x), std::abs(v.y), std::abs(v.z));
	}
}

// Constants

constexpr rt::vector ZERO      (0, 0, 0);

constexpr rt::vector RIGHT	   (1, 0, 0);
constexpr rt::vector LEFT	  (-1, 0, 0);

constexpr rt::vector UP		  (0,  1, 0);
constexpr rt::vector DOWN	  (0, -1, 0);


// FORWARD = positive z, toward the camera
constexpr rt::vector FORWARD  (0, 0, 1);
// BACKWARD = negative z, same direction as the camera
constexpr rt::vector BACKWARD (0, 0, -1);





