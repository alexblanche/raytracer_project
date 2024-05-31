#pragma once

/* Structure that contains information for barycentric coordinates
   l1, l2: ST-coordinates (coordinates in object space)
   lower_triangle: true if the point lies in the lower_triangle (p0, p1, p2) of a quad */

struct barycentric_info {
    double l1;
    double l2;
    bool lower_triangle;

    barycentric_info(const double& l1, const double& l2, bool lower_triangle)
        : l1(l1), l2(l2), lower_triangle(lower_triangle) {}

    barycentric_info(const double& l1, const double& l2)
        : l1(l1), l2(l2), lower_triangle(true) {}
};