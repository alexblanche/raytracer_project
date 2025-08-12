#include "tracing/directions.hpp"
#include "tracing/tracing.hpp"

/* ******************************************************************** */
/* Path tracing with multisample approach:
   After the first hit, multiple rays are cast */

/* Auxiliary function that updates the color accumulators */
inline void update_accumulators(
    const material& m, rt::color& emitted_colors, rt::color& color_materials,
    const rt::color& local_color,
    const bool update_color_materials) {

    /*
    if (m.is_emissive()) {
        const rt::color emitted_light = m.get_emitted_color() * m.get_emission_intensity();
        //emitted_colors = emitted_colors + (color_materials * emitted_light);
        emitted_colors += color_materials * emitted_light;
        //emitted_colors = fma(color_materials, emitted_light, emitted_colors);
    }
    
    if (update_color_materials) {
        //color_materials = color_materials * local_color;
        color_materials *= local_color;
    }
    */
   emitted_colors = m.is_emissive() ? fma(color_materials, m.get_emitted_color() * m.get_emission_intensity(), emitted_colors) : emitted_colors;
   color_materials = update_color_materials ? color_materials * local_color : color_materials;
}

rt::vector random_dir(randomgen& rg, const rt::vector central_dir, const real scattering) {
    
    return (central_dir + (1.0f - scattering) * random_direction(rg, central_dir, PI)).unit();
}

rt::vector compute_bias(const rt::vector& hit_point, const rt::vector& normal,
    const bool inward, const bool outward_bias) {

    return hit_point + ((inward == outward_bias) ? (1.0E-3f)*normal : (-1.0E-3f)*normal);
}

#define FIRST true
#define SECOND false

/* Computation of the new central direction and scattering */
void compute_bouncing_ray(const material& m, const hit& h,
    // Output
    rt::vector& orig1, rt::vector& orig2, rt::vector& central_dir1, std::optional<rt::vector>& central_dir2, real& scattering1, real& scattering2,
    real& proba_1, // if rg.random_real(1.0f) <= proba_1, choose central_dir1 with scattering1, otherwise central_dir2, scattering2
    rt::color& color_materials1, rt::color& color_materials2, rt::color& emitted_colors, real& init_refr_index) {

    init_refr_index = 1.0f;

    color_materials1 = rt::color::WHITE;
    color_materials2 = rt::color::WHITE;
    emitted_colors = rt::color::BLACK;

    auto update_acc = [&](bool reflects_colors, const rt::color& local_color, bool first) {
        rt::color& color_materials = (first) ? color_materials1 : color_materials2;
        update_accumulators(m, emitted_colors, color_materials, local_color, reflects_colors);
    };

    const real inward = h.is_inward();

    if (m.is_opaque()) {
        /* Diffuse or specular reflection */

        if (m.has_specular_proba()) {
            // Mix of specular and diffuse
            central_dir1 = get_central_reflected_direction(h, h.get_normal(), m.get_reflectivity(), inward);
            scattering1 = m.get_reflectivity();
            orig1 = compute_bias(h.get_point(), h.get_normal(), inward, true);
            update_acc(m.does_reflect_color(), m.get_color(), FIRST);

            proba_1 = m.get_specular_proba();

            const rt::vector& normal = h.get_normal();
            central_dir2 = inward ? normal : (-1.0f) * normal;
            scattering2 = 0.0f;
            orig2 = compute_bias(h.get_point(), normal, inward, true);
            update_acc(true, m.get_color(), SECOND);
        }
        else {
            // Only diffuse
            const rt::vector& normal = h.get_normal();
            central_dir1 = inward ? normal : (-1.0f) * normal;
            scattering1 = 0.0f;
            orig1 = compute_bias(h.get_point(), normal, inward, true);

            update_acc(true, m.get_color(), FIRST);
        }
    }
    else {
        /* Transmission or reflection, depending on the Fresnel coefficients Kr, Kt
            Kr is the probability that the ray is reflected, Kt the probability that the ray is transmitted */

        /* Computation of the new refraction index */
        const real next_refr_i = inward ? m.get_refraction_index() : 1.0f;

        /* Pre-computation of the refracted direction */
        real sin_theta_2_sq;
        const rt::vector vx = get_sin_refracted(h, h.get_normal(), 1.0f, next_refr_i, sin_theta_2_sq);

        /* Computation of the Fresnel coefficient */
        const real kr = inward ? get_schlick(h, h.get_normal(), 1.0f, next_refr_i) : 0.0f;

        if (sin_theta_2_sq >= 1.0f) {
            // Total internal reflection

            central_dir1 = get_central_reflected_direction(h, h.get_normal(), m.get_reflectivity(), inward);
            scattering1 = m.get_reflectivity();
            orig1 = compute_bias(h.get_point(), h.get_normal(), inward, true);

            update_acc(false, m.get_color(), FIRST);
        }
        else {
            // Mix of reflection and transmission

            // Transmission
            central_dir1 = get_refracted_direction(h.get_normal(), vx, sin_theta_2_sq, inward);
            scattering1 = 1.0f - m.get_refraction_scattering();

            init_refr_index = next_refr_i;
            orig1 = compute_bias(h.get_point(), h.get_normal(), inward, false);
            update_acc(true, m.get_color(), FIRST);

            if (inward) {
                // Reflection
                central_dir2 = get_central_reflected_direction(h, h.get_normal(), m.get_reflectivity(), inward);
                scattering2 = m.get_reflectivity();
                orig2 = compute_bias(h.get_point(), h.get_normal(), inward, true);

                update_acc(false, m.get_color(), SECOND);

                proba_1 = 1.0f - std::min(std::max(kr / m.get_transparency(), (real) 0.0f), (real) 1.0f);
            }
        }
    }
}

rt::color pathtrace_multisample(ray& r, scene& scene, randomgen& rg, const unsigned int bounce, const unsigned int number_of_samples) {
    
    const bool bounding_method = scene.polygons_per_bounding != 0;

    const std::optional<hit> opt_h =
        bounding_method ?
            scene.find_closest_object_bounding(r)
            :
            scene.find_closest_object(r);

    if (opt_h.has_value()) {
        const hit& h = opt_h.value();
        const object* obj = h.get_object();
        const material& m = scene.material_set[obj->get_material_index()];

        if (m.is_emissive() && m.get_emission_intensity() >= 1.0f) {

            if (obj->is_textured()) {
                // Only polygons (triangles and quads) can be textured (for now)
                
                const barycentric_info bary = obj->get_barycentric(h.get_point());
                return
                    scene.sample_texture(obj->get_texture_info(), bary)
                        * m.get_emission_intensity();
            }
            else {
                return m.get_emitted_color() * m.get_emission_intensity();
            }                
        }

        // Multiple samples cast
        
        /* Computation of the new origin, central direction and scattering */
        rt::color color_materials1;
        rt::color color_materials2;
        rt::color emitted_colors;
        rt::vector orig1;
        rt::vector orig2;
        rt::vector central_dir1;
        std::optional<rt::vector> central_dir2;
        real scattering1;
        real scattering2;
        real proba_1 = 1.0f;
        real init_refr_index;
        compute_bouncing_ray(m, h,
            orig1, orig2, central_dir1, central_dir2, scattering1, scattering2, proba_1,
            color_materials1, color_materials2, emitted_colors, init_refr_index);

        const bool one_direction = (not central_dir2.has_value()) || (proba_1 == 1.0f);

        if (one_direction) {
            // Accumulator
            rt::color output_color = rt::color::BLACK;

            for (unsigned int i = 0; i < number_of_samples; i++) {

                const rt::vector dir = random_dir(rg, central_dir1, scattering1);
                ray bouncing_ray(orig1, dir);
                const rt::color sample_color = pathtrace(bouncing_ray, scene, rg, bounce - 1, 1.0f);
                output_color = output_color + sample_color;
            }

            return color_materials1 * (output_color / number_of_samples) + emitted_colors;
        }
        else {
            // Accumulators
            rt::color output_color1 = rt::color::BLACK;
            rt::color output_color2 = rt::color::BLACK;
            unsigned int nb_samples1 = 0;
            unsigned int nb_samples2 = 0;

            for (unsigned int i = 0; i < number_of_samples; i++) {

                const real p = rg.random_real(1.0f);
                if (p <= proba_1) {
                    const rt::vector dir = random_dir(rg, central_dir1, scattering1);
                    ray bouncing_ray(orig1, dir);
                    const rt::color sample_color = pathtrace(bouncing_ray, scene, rg, bounce - 1, init_refr_index);
                    output_color1 = output_color1 + sample_color;
                    nb_samples1++;
                }
                else {
                    const rt::vector dir = random_dir(rg, central_dir2.value(), scattering2);
                    ray bouncing_ray(orig2, dir);
                    const rt::color sample_color = pathtrace(bouncing_ray, scene, rg, bounce - 1, 1.0f);
                    output_color2 = output_color2 + sample_color;
                    nb_samples2++;
                }
            }

            rt::color output_color = emitted_colors;
            if (nb_samples1) output_color = output_color + (color_materials1 * ((output_color1 / nb_samples1) * (((float) nb_samples1) / number_of_samples)));
            if (nb_samples2) output_color = output_color + (color_materials2 * ((output_color2 / nb_samples2) * (((float) nb_samples2) / number_of_samples)));
            return output_color;
        }
    }
    else {
        return
            (scene.background.has_texture()) ?
                scene.background.get_color(r.get_direction())
                :
                scene.background.get_color();
    }
}