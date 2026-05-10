#pragma once

#include <vector>
#include "screen/color.hpp"

class material {
    
    private:
    
        /* Color of the material */
        rt::color color;

        /* Real between 0 and 1: 0 is a pure diffuse surface, 1 is a pure mirror */
        real smoothness;

        /* Real between 0 and 1: 0 does not emit light, 1 is maximum intensity */
        real emission_intensity;

        /* Probability of a specular bounce */
        real reflectivity;

        /* Transparency: real between 0 and 1 */
        real transparency;
        
        /* Scattering of the refracted rays (similar to glossiness for reflected rays) */
        real refraction_scattering;

        /* Indice of refraction: air = 1, water = 1.3, glass = 1.5, diamond = 1.8 */
        real refraction_index;

        /* Booleans to speed up the tracing function */
        bool opaque;
        bool emissive;
        bool has_specularity;

        /* True if the material color is reflected in specular bounces, false if it is white */
        bool reflects_color;

    public:

        constexpr material()
            :   color(rt::WHITE), smoothness(0),
                emission_intensity(0),
                reflectivity(0),
                transparency(0),
                refraction_scattering(0),
                refraction_index(1),
                opaque(true), emissive(false), has_specularity(false),
                reflects_color(false) {}

        constexpr material(const rt::color& color, const real smoothness)
            :   color(color), smoothness(smoothness),
                emission_intensity(0),
                reflectivity(0),
                transparency(0),
                refraction_scattering(0),
                refraction_index(1),
                opaque(true), emissive(false), has_specularity(false), 
                reflects_color(false) {}

        constexpr material(const rt::color& color,
            const real smoothness, const real emission_intensity,
            const real reflectivity, const bool reflects_color,
            const real transparency, const real refraction_scattering,
            const real refraction_index)
                :   color(color), smoothness(smoothness),
                    emission_intensity(emission_intensity),
                    reflectivity(reflectivity),
                    transparency(transparency),
                    refraction_scattering(refraction_scattering),
                    refraction_index(refraction_index),
                    opaque(transparency == 0), emissive(emission_intensity != 0), has_specularity(reflectivity != 0),
                    reflects_color(reflects_color) {}

        /* Constructor from mtl parameters */
        material(const real ns,
            const rt::color& ka, const rt::color& kd, const rt::color& ks, const rt::color& ke,
            const real ni, const real d, const unsigned int illum, const real gamma);


        material(material&&)                 = default;
        material& operator=(material&&)      = default;
        material(const material&)            = default;

        material& operator=(const material&) = delete;

        inline const rt::color& get_color() const {
            return color;
        }

        inline real get_smoothness() const {
            return smoothness;
        }

        inline real get_emission_intensity() const {
            return emission_intensity;
        }

        inline real get_reflectivity() const {
            return reflectivity;
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

        inline bool is_specular() const {
            return has_specularity;
        }
};

constexpr material DIFFUSE(rt::WHITE, 0);
constexpr material MIRROR (rt::WHITE, 1, 0, 1, false, 0,    0, 1.0 );
constexpr material GLASS  (rt::WHITE, 1, 0, 1, false, 0.95, 0, 1.52);
constexpr material WATER  (rt::WHITE, 1, 0, 1, false, 1,    0, 1.33);

/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color);

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const real emission_intensity);