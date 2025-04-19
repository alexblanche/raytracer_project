#include "scene/light_sources/infinite_area.hpp"

#include <cmath>
#include <stack>

#define PI 3.14159265358979323846f

/* Infinite area light sample */

std::vector<real> compute_low_res_table(std::vector<std::vector<rt::color>>& matrix) {

    const unsigned int width  = std::min((size_t) LOWRES_DEFAULT_WIDTH, matrix.size());
    const unsigned int height = std::min((size_t) LOWRES_DEFAULT_HEIGHT, matrix[0].size());

    const real ratio_x = (matrix.size() * 1.0f) / LOWRES_DEFAULT_WIDTH;
    const real ratio_y = (matrix[0].size() * 1.0f) / LOWRES_DEFAULT_HEIGHT;

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

alias_table::alias_table(const std::vector<real> prob_table,
    const unsigned int map_width,
    const unsigned int map_height,
    const unsigned int pt_width,
    const unsigned int pt_height
    )
    
    :
        map_width(map_width),
        map_height(map_height),
        pt_width(pt_width),
        //pt_height(pt_height),
        ratio_x(map_width * 1.0f / pt_width),
        ratio_y(map_height * 1.0f / pt_height) {

    const real n = prob_table.size();
    const real invn = 1.0f / n;
    bins.resize(n);
    
    std::stack<alias_bin> under;
    std::stack<alias_bin> over;
    
    // Partition the bins into the under and over 1/n
    
    for (unsigned int i = 0; i < n; i++) {
        alias_bin ab(prob_table[i], i);
        if (prob_table[i] < invn)
            under.push(ab);
        else
            over.push(ab);
    }

    while (not (under.empty() || over.empty())) {
        alias_bin& ub = under.top();
        alias_bin& ob = over.top();

        // Setting the final value of u
        alias_bin& final_u = bins[ub.alias];
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
        bins[ub.alias].p = 1;
        // For safety
        bins[ub.alias].alias = ub.alias;
        under.pop();
    }
    while (not over.empty()) {
        alias_bin& ob = over.top();
        bins[ob.alias].p = 1;
        // For safety
        bins[ob.alias].alias = ob.alias;
        over.pop();
    }
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

unsigned int alias_table::sample(randomgen& rg) {

    const unsigned int i = rg.random_real(bins.size());

    alias_bin& bin = bins[i];
    if (bin.p == 1.0f || rg.random_real(1.0f) < bin.p)
        return i;
    else
        return bin.alias;
}

// Returns the coordinates of a pixel in the light map, chosen according to the probability from the table
std::pair<unsigned int, unsigned int> alias_table::get_sample_for_light_map(randomgen& rg) {

    const unsigned int s = sample(rg);
    // s corresponds to a pixel in the low-res image

    const unsigned int lr_x = s % pt_width;
    const unsigned int lr_y = s / pt_width;

    const unsigned int min_x = ratio_x * lr_x;
    const unsigned int max_x = ratio_x * (lr_x + 1);
    
    const unsigned int min_y = ratio_y * lr_y;
    const unsigned int max_y = ratio_y * (lr_y + 1);

    const unsigned int x = min_x + rg.random_real(max_x - min_x);
    const unsigned int y = min_y + rg.random_real(max_y - min_y);
    return std::pair<unsigned int, unsigned int>(x, y);
}