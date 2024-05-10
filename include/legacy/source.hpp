#pragma once

#include "light/vector.hpp"
#include "screen/color.hpp"

#include "light/hit.hpp"
#include "scene/objects/object.hpp"

class source {
    private:
        rt::vector position;
        rt::color color;

    public:
        rt::color apply_obj(const hit& h, const vector<const object*>& obj_set) const;

};