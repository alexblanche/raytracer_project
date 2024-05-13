#pragma once

#include "light/vector.hpp"
#include "screen/color.hpp"
#include "legacy/light/hit.hpp"
#include "legacy/objects/object.hpp"

class source {
    private:
        rt::vector position;
        rt::color color;

    public:
        source();

        source(const rt::vector& position, const rt::color& color);

        rt::color apply_obj(const hit& h, const vector<const object*>& obj_set) const;
};