#pragma once

#include <vector>
#include "screen/color.hpp"

class material {
    
    private:

        /*** Reflectivity ***/
    
        /* Color of the material */
        rt::color color;

        /* Real between 0 and 1: 0 is a pure diffuse surface, 1 is a pure mirror */
        double reflectivity;

        /* Color emitted by the material */
        rt::color emitted_color;

        /* Real between 0 and 1: 0 does not emit light, 1 is maximum intensity */
        double emission_intensity;


        /*** Specular bounce ***/

        /* Probability of a specular bounce */
        double specular_probability;

        /* True if the material color is reflected in specular bounces, false if it is white */
        bool reflects_color;


        /*** Transparency ***/

        /* Transparency: real between 0 and 1 */
        double transparency;
        
        /* Scattering of the refracted rays (similar to glossiness for reflected rays) */
        double refraction_scattering;

        /* Indice of refraction: air = 1, water = 1.3, glass = 1.5, diamond = 1.8 */
        double refraction_index;

    public:

        /* Mirror surface */
        static material DIFFUSE;
        static material MIRROR;
        static material GLASS;
        static material WATER;


        /* Constructors */

        material();

        /* Constructs a material with no emitted light, with specular probability 1 */
        material(const rt::color& color, const double& reflectivity);

        /* Main constructor */
        material(const rt::color& color, const rt::color& emitted_color,
            const double& reflectivity, const double& emission_intensity,
            const double& specular_probability, const bool reflects_color,
            const double& transparency, const double& refraction_scattering,
            const double& refraction_index_in);

        /* Constructor from mtl parameters */
        material(const double& ns,
            const rt::color& ka, const rt::color& kd, const rt::color& ks, const rt::color& ke,
            const double& ni, const double& d, const unsigned int illum);

        material(const material&) = delete;

        material& operator=(const material&) = delete;

        material(material&&) = default;

        material& operator=(material&&) = default;


        /* Accessors */

        inline rt::color get_color() const {
            return color;
        }

        inline rt::color get_emitted_color() const {
            return emitted_color;
        }

        inline double get_reflectivity() const {
            return reflectivity;
        }

        inline double get_emission_intensity() const {
            return emission_intensity;
        }

        inline double get_specular_proba() const {
            return specular_probability;
        }

        inline bool does_reflect_color() const {
            return reflects_color;
        }

        inline double get_transparency() const {
            return transparency;
        }
        
        inline double get_refraction_scattering() const {
            return refraction_scattering;
        }

        inline double get_refraction_index() const {
            return refraction_index;
        }
};

/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color);

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const double& emission_intensity);