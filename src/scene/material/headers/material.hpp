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

        // texture, specular_probability, ...

    public:

        /* Constructors */
        material();

        material(const rt::color& color, const rt::color& emitted_color,
            const double reflectivity, const double emission_intensity);


        /* Accessors */

        rt::color get_color() const;

        rt::color get_emitted_color() const;

        double get_reflectivity() const;

        double get_emission_intensity() const;

};