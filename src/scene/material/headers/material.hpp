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

        // texture
        // transparency

    public:

        /* Mirror surface */
        static const material MIRROR;


        /* Constructors */

        material();

        /* Constructs a material with no emitted light, with specular probability 1 */
        material(const rt::color& color, const double reflectivity);

        /* Main constructor */
        material(const rt::color& color, const rt::color& emitted_color,
            const double reflectivity, const double emission_intensity,
            const double specular_probability, const bool reflects_color);



        /* Accessors */

        rt::color get_color() const;

        rt::color get_emitted_color() const;

        double get_reflectivity() const;

        double get_emission_intensity() const;

        double get_specular_proba() const;

        bool does_reflect_color() const;
};

/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color);

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const double emission_intensity);
