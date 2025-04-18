#pragma once

#include <vector>
#include "screen/color.hpp"
#include <cmath>
#include <stack>

#define PI 3.14159265358979323846f

/* Infinite area light sample */

std::vector<real> compute_low_res_table(std::vector<std::vector<rt::color>>& matrix) {

    constexpr unsigned int DEFAULT_WIDTH = 854;
    constexpr unsigned int DEFAULT_HEIGHT = 480;

    const unsigned int width  = std::min((size_t) DEFAULT_WIDTH, matrix.size());
    const unsigned int height = std::min((size_t) DEFAULT_HEIGHT, matrix[0].size());

    const real ratio_x = (matrix.size() * 1.0f) / DEFAULT_WIDTH;
    const real ratio_y = (matrix[0].size() * 1.0f) / DEFAULT_HEIGHT;

    std::vector<real> table(width * height);
    for (unsigned int ly = 0; ly < height; ly++) {

        const real phi = ly * PI / height;
        const real sinphi = sin(phi);
        const unsigned int l = ly * width;

        for (unsigned int i = l; i < l + width; i++) {
            table[i] = sinphi;
        }
    }

    for (unsigned int i = 0; i < table.size(); i++) {

        const unsigned int lx = i % width;
        const unsigned int ly = i / width;

        // Sampling the high res matrix and averaging the pixels
        rt::color sum(0.0f, 0.0f, 0.0f);
        for (unsigned int x = lx * ratio_x; x < (lx+1) * ratio_x; x++) {
            for (unsigned int y = ly * ratio_y; y < (ly+1) * ratio_y; y++) {

                sum = sum + matrix[x][y];
            }
        }
        sum = sum / ((((lx+1) * ratio_x) - (lx * ratio_x)) * (((ly+1) * ratio_y) - (ly * ratio_y)));

        table[i] *= sum.get_average();
    }

    // Normalize the table
    real weight_sum = 0.0f;
    for (unsigned int i = 0; i < table.size(); i++) {
        weight_sum += table[i];
    }
    for (unsigned int i = 0; i < table.size(); i++) {
        table[i] /= weight_sum;
    }
    
    return table;
}

struct alias_bin {
    real p;
    unsigned int alias;

    alias_bin() : p(0.0f), alias(0) {}
    alias_bin(const real p, const unsigned int alias)
        : p(p), alias(alias) {}
};

std::vector<alias_bin> compute_alias_table(const std::vector<real> lrt) {

    const real n = lrt.size();
    const real invn = 1.0f / n;
    std::vector<alias_bin> alias_table(n);
    
    std::stack<alias_bin> under;
    std::stack<alias_bin> over;
    
    // Partition the bins into the under and over 1/n
    
    for (unsigned int i = 0; i < n; i++) {
        alias_bin ab(lrt[i], i);
        if (lrt[i] < invn)
            under.push(ab);
        else
            over.push(ab);
    }

    while (not (under.empty() || over.empty())) {
        alias_bin& ub = under.top();
        alias_bin& ob = over.top();

        // Setting the final value of u
        alias_bin& final_u = alias_table[ub.alias];
        final_u.p = n * ub.p;
        final_u.alias = ob.alias;

        under.pop();

        // Substracting the excess probability of o
        ob.p -= invn * (1 - final_u.p);
        if (ob.p < invn) {
            // No longer belongs to over
            alias_bin new_ob(ob.p, ob.alias);
            under.push(new_ob);
            over.pop();
        }
    }

    // Remaining bins should be set to probability 1
    while (not under.empty()) {
        alias_bin& ub = under.top();
        alias_table[ub.alias].p = 1;
        // For safety
        alias_table[ub.alias].alias = ub.alias;
        under.pop();
    }
    while (not over.empty()) {
        alias_bin& ob = over.top();
        alias_table[ob.alias].p = 1;
        // For safety
        alias_table[ob.alias].alias = ob.alias;
        over.pop();
    }

    return alias_table;
}

/*
    Examples

    { 0.1, 0.7, 0.2 }

    1/3 -> 0.3 | 2          (1 a prob 1/3 * 0.3 = 0.1, 2 a prob 1/3 * 0.7 = 0.2333333)

    1/3 -> 1 | _            (2 a maintenant prob (1/3 * 0.7) + 1/3 = 0.56666666666)

    1/3 -> 0.6 | 2          (2 a prob (1/3 * (0.7 + 1 + 0.4) = 0.7, 3 a prob (1/3 * 0.6 = 0.2))



    { 0.1, 0.2, 0.7 }

    1/3 -> 0.3 | 3          (1 a prob 1/3 * 0.3 = 0.1, 3 a prob 1/3 * 0.7 = 0.2333333)

    1/3 -> 0.6 | 3          (2 a prob (1/3 * 0.6) = 0.2, 3 a prob 0.2333333 + 1/3 * 0.4 = 0.36666666666)

    1/3 -> 1   | _          (3 a prob 0.36666666666 + (1/3 * 1) = 0.7)
*/

