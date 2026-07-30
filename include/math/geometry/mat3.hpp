#pragma once

#include "math/geometry/vector.hpp"

namespace linalg {

    enum class mat_type {
        Row, Col
    };
    using enum mat_type;

    template<mat_type type>
    class mat3 {};

    template<>
    class mat3<Row> {
        public:
            rt::vector r1, r2, r3;

            rt::vector operator*(const rt::vector& v) const {
                return rt::vector(
                    (r1 | v),
                    (r2 | v),
                    (r3 | v)
                );
            }

            mat3 operator*(const mat3& m) const {

                const rt::vector c[] = {
                    rt::vector(m.r1.x, m.r2.x, m.r3.x),
                    rt::vector(m.r1.y, m.r2.y, m.r3.y),
                    rt::vector(m.r1.z, m.r2.z, m.r3.z)
                };
                const auto& [ c1, c2, c3 ] = c;

                return mat3 {
                    .r1 = (*this) * c1,
                    .r2 = (*this) * c2,
                    .r3 = (*this) * c3
                };
            }

            static mat3 rotation_x(const real theta_x) {
                const real cos_x = cos(theta_x);
                const real sin_x = sin(theta_x);
                return mat3 {
                    .r1 = rt::vector(1,     0,      0),
                    .r2 = rt::vector(0, cos_x, -sin_x),
                    .r3 = rt::vector(0, sin_x,  cos_x)
                };
            }

            static mat3 rotation_y(const real theta_y) {
                const real cos_y = cos(theta_y);
                const real sin_y = sin(theta_y);
                return mat3 {
                    .r1 = rt::vector( cos_y, 0, sin_y),
                    .r2 = rt::vector(     0, 1,     0),
                    .r3 = rt::vector(-sin_y, 0, cos_y)
                };
            }

            static mat3 rotation_z(const real theta_z) {
                const real cos_z = cos(theta_z);
                const real sin_z = sin(theta_z);
                return mat3 {
                    .r1 = rt::vector(cos_z, -sin_z, 0),
                    .r2 = rt::vector(sin_z,  cos_z, 0),
                    .r3 = rt::vector(    0,      0, 1)
                };
            }

            /* Returns the matrix rotation_z(theta_z) * rotation_y(theta_y) * rotation_x(theta_x) */
            static mat3 rotation(const real theta_x, const real theta_y, const real theta_z) {
                const real cos_x = cos(theta_x);
                const real sin_x = sin(theta_x);
                
                const real cos_y = cos(theta_y);
                const real sin_y = sin(theta_y);
                
                const real cos_z = cos(theta_z);
                const real sin_z = sin(theta_z);

                return mat3 {
                    .r1 = rt::vector(
                        cos_y * cos_z,
                        cos_z * sin_x * sin_y - cos_x * sin_z,
                        cos_x * cos_z * sin_y + sin_x * sin_z
                    ),
                    .r2 = rt::vector(
                        cos_y * sin_z,
                        cos_x * cos_z + sin_x * sin_y * sin_z,
                        cos_x * sin_y * sin_z - cos_z * sin_x
                    ),
                    .r3 = rt::vector(
                        -sin_y,
                        cos_y * sin_x,
                        cos_x * cos_y
                    )
                };
            }

            /* Returns the inverse of rotation(theta_x, theta_y, theta_z) */
            static mat3 inverse_rotation(const real theta_x, const real theta_y, const real theta_z) {
                return rotation_x(-theta_x) * rotation_y(-theta_y) * rotation_z(-theta_z);
            }
    };

    template<>
    class mat3<Col> {
        public:
            rt::vector c1, c2, c3;

            rt::vector operator*(const rt::vector& v) const {
                return rt::matprod(c1, c2, c3, v);
            }

            mat3 operator*(const mat3& m) const {
                return mat3 {
                    .c1 = (*this) * m.c1,
                    .c2 = (*this) * m.c2,
                    .c3 = (*this) * m.c3
                };
            }

            static mat3 rotation_x(const real theta_x) {
                const real cos_x = cos(theta_x);
                const real sin_x = sin(theta_x);
                return mat3 {
                    .c1 = rt::vector(1,      0,     0),
                    .c2 = rt::vector(0,  cos_x, sin_x),
                    .c3 = rt::vector(0, -sin_x, cos_x)
                };
            }

            static mat3 rotation_y(const real theta_y) {
                const real cos_y = cos(theta_y);
                const real sin_y = sin(theta_y);
                return mat3 {
                    .c1 = rt::vector(cos_y, 0, -sin_y),
                    .c2 = rt::vector(    0, 1,      0),
                    .c3 = rt::vector(sin_y, 0,  cos_y)
                };
            }

            static mat3 rotation_z(const real theta_z) {
                const real cos_z = cos(theta_z);
                const real sin_z = sin(theta_z);
                return mat3 {
                    .c1 = rt::vector( cos_z, sin_z, 0),
                    .c2 = rt::vector(-sin_z, cos_z, 0),
                    .c3 = rt::vector(     0,     0, 1)
                };
            }

            /* Returns the matrix rotation_z(theta_z) * rotation_y(theta_y) * rotation_x(theta_x) */
            static mat3 rotation(const real theta_x, const real theta_y, const real theta_z) {

                // return rotation_z(theta_z) * rotation_y(theta_y) * rotation_x(theta_x);

                const real cos_x = cos(theta_x);
                const real sin_x = sin(theta_x);
                
                const real cos_y = cos(theta_y);
                const real sin_y = sin(theta_y);
                
                const real cos_z = cos(theta_z);
                const real sin_z = sin(theta_z);

                return mat3 {
                    .c1 = rt::vector(
                        cos_y * cos_z,
                        cos_y * sin_z,
                        -sin_y
                    ),
                    .c2 = rt::vector(
                        cos_z * sin_x * sin_y - cos_x * sin_z,
                        cos_x * cos_z + sin_x * sin_y * sin_z,
                        cos_y * sin_x    
                    ),
                    .c3 = rt::vector(
                        cos_x * cos_z * sin_y + sin_x * sin_z,
                        cos_x * sin_y * sin_z - cos_z * sin_x,
                        cos_x * cos_y
                    )
                };
            }

            /* Returns the inverse of rotation(theta_x, theta_y, theta_z) */
            static mat3 inverse_rotation(const real theta_x, const real theta_y, const real theta_z) {
                return rotation_x(-theta_x) * rotation_y(-theta_y) * rotation_z(-theta_z);
            }
    };
}