#include "scene/light_sources/infinite_area.hpp"

#include "auxiliary/custom_stack.hpp"

#include <cmath>
#include <span>

/* Infinite area light sample */

std::vector<real> alias_table::compute_low_res_table(const matrix& matrix) {

    const int width  = std::min(LOWRES_DEFAULT_WIDTH,  static_cast<unsigned int>(matrix.width));
    const int height = std::min(LOWRES_DEFAULT_HEIGHT, static_cast<unsigned int>(matrix.height));

    const real ratio_x = static_cast<real>(matrix.width)  / LOWRES_DEFAULT_WIDTH;
    const real ratio_y = static_cast<real>(matrix.height) / LOWRES_DEFAULT_HEIGHT;
    const real r = PI / static_cast<real>(height);

    const int table_size = width * height;
    std::vector<real> table(table_size);

    real phi = 0.0_r;
    for (int ly = 0; ly < height; ly++) {

        const real sinphi = sin(phi);
        const int l = ly * width;
        const int bound = l + width;

        for (int i = l; i < bound; i++)
            table[i] = sinphi;
        phi += r;
    }

    for (int i = 0; i < table_size; i++) {

        const int lx = i % width;
        const int ly = i / width;

        // Sampling the high res matrix and averaging the pixels
        const int init_x  =  lx      * ratio_x;
        const int bound_x = (lx + 1) * ratio_x;
        const int init_y  =  ly      * ratio_y;
        const int bound_y = (ly + 1) * ratio_y;
        rt::color sum;
        for (int x = init_x; x < bound_x; x++) {
            for (int y = init_y; y < bound_y; y++) {
                sum += matrix[y, x];
            }
        }
        sum /= (bound_x - init_x) * (bound_y - init_y);
        table[i] *= sum.get_average();
    }

    // Normalize the table
    real weight_sum = 0.0_r;
    for (const real x : table)
        weight_sum += x;

    for (real& x : table)
        x /= weight_sum;
    
    return table;
}

alias_table::alias_table(const std::vector<real>& prob_table,
    const unsigned int map_width,
    const unsigned int map_height,
    const unsigned int pt_width,
    const unsigned int pt_height)
    
    :
        nb_bins(static_cast<real>(prob_table.size())),
        map_width(map_width),
        map_height(map_height),
        pt_width(pt_width),
        //pt_height(pt_height),
        ratio_x(static_cast<real>(map_width)  / pt_width),
        ratio_y(static_cast<real>(map_height) / pt_height) {

    const int n = prob_table.size();
    bins.resize(n);
    const real invn = 1.0_r / nb_bins;
    
    custom_stack<alias_bin> under;
    custom_stack<alias_bin> over;
    
    // Partition the bins into the under and over 1/n
    
    for (int i = 0; real p : prob_table) {
        auto& stack = (p < invn) ? under : over;
        stack.emplace(p, i);
        i++;
    }

    while (not (under.empty() || over.empty())) {
        const alias_bin& ub = under.top();
        alias_bin& ob = over.top();

        // Setting the final value of u
        auto& [ p, alias ] = bins[ub.alias];
        p = nb_bins * ub.p;
        alias = ob.alias;

        under.pop();

        // Substracting the excess probability of o
        ob.p -= invn * (1.0_r - p);
        if (ob.p < invn) {
            // No longer belongs to over
            under.emplace(ob);
            over.pop();
        }
    }

    // Remaining bins should be set to probability 1
    auto& b = bins;
    auto handle_remaining = [&b] (custom_stack<alias_bin>& stack) {
        const auto content = stack.get_content();
        for (const auto& [ _ , alias] : content)
            b[alias] = { 1.0_r, alias };
        stack.set_empty();
    };
    handle_remaining(under);
    handle_remaining(over);
}

alias_table::alias_table(const matrix& matrix,
    const unsigned int pt_width,
    const unsigned int pt_height)

    : alias_table(alias_table::compute_low_res_table(matrix), matrix.width, matrix.height, pt_width, pt_height) {}

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

// Returns the coordinates of a pixel in the light map, chosen according to the probability from the table
light_map_sample alias_table::get_sample_for_light_map(const randomgen& rg) const {

    const unsigned int s = sample(rg);
    // s corresponds to a pixel in the low-res image

    const real lr_x = static_cast<real>(s % pt_width);
    const real lr_y = static_cast<real>(s / pt_width);

    const unsigned int min_x = static_cast<unsigned int>(ratio_x *  lr_x);
    const unsigned int max_x = static_cast<unsigned int>(ratio_x * (lr_x + 1.0_r));
    
    const unsigned int min_y = static_cast<unsigned int>(ratio_y *  lr_y);
    const unsigned int max_y = static_cast<unsigned int>(ratio_y * (lr_y + 1.0_r));

    const unsigned int x = min_x + rg.random_real(max_x - min_x);
    const unsigned int y = min_y + rg.random_real(max_y - min_y);
    return { x, y };
}