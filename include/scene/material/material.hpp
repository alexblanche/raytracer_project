#pragma once

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


        /*** Texture (optional) ***/
        
        /* Boolean indicating whether a texture is specified */
        bool textured;

        /* Texture index in texture::set */
        unsigned int texture_index;

        /* Vector of UV coordinates (between 0 and 1)
           6 for a triangle (u0,v0,u1,v1,u2,v2) and 8 for a quad */
        std::vector<double> uv_coordinates;



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
            const double& refraction_index_in);

        /* Main constructor with added texture */
        material(const rt::color& color, const rt::color& emitted_color,
            const double& reflectivity, const double& emission_intensity,
            const double& specular_probability, const bool reflects_color,
            const double& transparency, const double& refraction_scattering,
            const double& refraction_index_in,
            const unsigned int texture_index, const std::vector<double>& uv_coordinates);



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

        bool is_textured() const {
            return textured;
        }


        /* Mutators */

        void set_texture(const unsigned int i, const std::vector<double>& uv_coord) {
            textured = true;
            texture_index = i;
            uv_coordinates = uv_coord;
        }


        /* Texturing */

        /* Write in u, v the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
        In the case of quads, the boolean lower_triangle indicates that the three points to
        consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
        void get_barycenter(const double& l1, const double& l2, const bool lower_triangle,
            double& u, double& v) const;

        /* Returns the color of the pixel associated with UV-coordinates u, v */
        rt::color get_texture_color(const double& l1, const double& l2, const bool lower_triangle) const;
};

/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color);

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const double emission_intensity);