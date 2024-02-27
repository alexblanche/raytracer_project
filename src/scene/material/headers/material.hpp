#pragma once

#include "../../../screen/headers/color.hpp"

class material {
    
    private:
    
        /* Color of the material */
        rt::color color;

        /* Color emitted by the material */
        rt::color emitted_color;

        /* Real between 0 and 1: 0 is a pure diffuse surface, 1 is a pure mirror */
        double reflectivity;

        /* Real between 0 and 1: 0 does not emit light, 1 is maximum intensity */
        double emission_intensity;

        /* Probability of a specular bounce */
        double specular_probability;

        /* True if the material color is reflected in specular bounces, false if it is white */
        bool reflects_color;

        /* Transparency: real between 0 and 1 */
        double transparency;
        
        /* Scattering of the refracted rays (similar to glossiness for reflected rays) */
        double refraction_scattering;

        /* Indice of refraction: air = 1, water = 1.3, glass = 1.5, diamond = 1.8 */
        double refraction_index;

        // Todo: texture


    public:

        /* Mirror surface */
        static const material MIRROR;
        static const material GLASS;
        // static const material WATER;


        /* Constructors */

        material();

        /* Constructs a material with no emitted light, with specular probability 1 */
        material(const rt::color& color, const double reflectivity);

        /* Main constructor */
        material(const rt::color& color, const rt::color& emitted_color,
            const double& reflectivity, const double& emission_intensity,
            const double& specular_probability, const bool reflects_color,
            const double& transparency, const double& refraction_scattering,
            const double& refraction_index_in
            );



        /* Accessors */

        rt::color get_color() const {
            return color;
        }

        rt::color get_emitted_color() const {
            return emitted_color;
        }

        double get_reflectivity() const {
            return reflectivity;
        }

        double get_emission_intensity() const {
            return emission_intensity;
        }

        double get_specular_proba() const {
            return specular_probability;
        }

        bool does_reflect_color() const {
            return reflects_color;
        }
};

/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color);

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const double emission_intensity);
