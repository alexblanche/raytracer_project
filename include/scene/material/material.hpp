#pragma once

#include <vector>
#include "screen/color.hpp"

class material {
    
    private:

        /*** Reflectivity ***/
    
        /* Color of the material */
        rt::color color;

        /* Real between 0 and 1: 0 is a pure diffuse surface, 1 is a pure mirror */
        real reflectivity;

        /* Color emitted by the material */
        // rt::color emitted_color;

        /* Real between 0 and 1: 0 does not emit light, 1 is maximum intensity */
        real emission_intensity;


        /*** Specular bounce ***/

        /* Probability of a specular bounce */
        real specular_probability;


        /*** Transparency ***/

        /* Transparency: real between 0 and 1 */
        real transparency;
        
        /* Scattering of the refracted rays (similar to glossiness for reflected rays) */
        real refraction_scattering;

        /* Indice of refraction: air = 1, water = 1.3, glass = 1.5, diamond = 1.8 */
        real refraction_index;

        /* Booleans to speed up the tracing function */
        bool opaque;
        bool emissive;
        bool has_spec_prob;

        /* True if the material color is reflected in specular bounces, false if it is white */
        bool reflects_color;

    public:

        /* Mirror surface */
        static material DIFFUSE;
        static material MIRROR;
        static material GLASS;
        static material WATER;


        /* Constructors */

        material();

        /* Constructs a material with no emitted light, with specular probability 1 */
        material(const rt::color& color, const real reflectivity);

        /* Main constructor */
        material(const rt::color& color, const rt::color& emitted_color,
            const real reflectivity, const real emission_intensity,
            const real specular_probability, const bool reflects_color,
            const real transparency, const real refraction_scattering,
            const real refraction_index_in);

        /* Constructor from mtl parameters */
        material(const real ns,
            const rt::color& ka, const rt::color& kd, const rt::color& ks, const rt::color& ke,
            const real ni, const real d, const unsigned int illum, const real gamma);

        material(const material&) = delete;

        material& operator=(const material&) = delete;

        material(material&&) = default;

        material& operator=(material&&) = default;


        /* Accessors */

        inline const rt::color& get_color() const {
            return color;
        }

        inline const rt::color& get_emitted_color() const {
            return color;//emitted_color;
        }

        inline real get_reflectivity() const {
            return reflectivity;
        }

        inline real get_secondary_reflectivity() const {
            return 0.f;
        }

        inline real get_emission_intensity() const {
            return emission_intensity;
        }

        inline real get_specular_proba() const {
            return specular_probability;
        }

        inline bool does_reflect_color() const {
            return reflects_color;
        }

        inline real get_transparency() const {
            return transparency;
        }
        
        inline real get_refraction_scattering() const {
            return refraction_scattering;
        }

        inline real get_refraction_index() const {
            return refraction_index;
        }

        inline bool is_opaque() const {
            return opaque;
        }

        inline bool is_emissive() const {
            return emissive;
        }

        inline bool has_specular_proba() const {
            return has_spec_prob;
        }
};

/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color);

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const real emission_intensity);