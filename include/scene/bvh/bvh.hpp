#pragma once

#include "scene/bvh/bounding.hpp"
#include "auxiliary/custom_stack.hpp"

class bvh {

    static_assert(std::is_same_v<bounding::box_type, box>
        || std::is_same_v<bounding::box_type, aabb>);
    
    private:

        /* Set of the top-level bounding boxes */
        std::vector<const bounding*>    bounding_set;

        /* Set of all bounding boxes */
        std::vector<bounding::box_type> box_set;

    public:

        unsigned int polygons_per_bounding = 0;

        enum class option {
            Enabled, Disabled
        };
        using enum option;

        option state = Disabled;

        bvh() {}

        bvh(const unsigned int polygons_per_bounding)
            : polygons_per_bounding(polygons_per_bounding), state(Enabled) {}

        void finalize(std::vector<const object*>&& content) {
            // content should be tested first, to maximize pruning in the BVH tree-search
            bounding_set.push_back(new bounding(std::move(content)));
            std::reverse(bounding_set.begin(), bounding_set.end());
        }

        /* Tree-search through the bounding boxes */
        std::optional<hit> find_closest_object(const ray& r) const;

        min_max_coord get_min_max_coord(const bounding* const bd) const {
            return (bd->box_index != EMPTY_INDEX) ?
                  box_set[bd->box_index].get_min_max_coord()
                : empty_set_min_max_coords;
        }

        bvh(bvh&&)            noexcept = default;
        bvh& operator=(bvh&&) noexcept = default;
        bvh(const bvh&)                = delete;
        bvh& operator=(const bvh&)     = delete;

        ~bvh() noexcept;

        void add_bounding(const bounding* bd) {
            bounding_set.push_back(bd);
        }

        /* Emplaces a new bounding box and returns its index */
        template<typename... Args>
        requires (std::is_constructible_v<bounding::box_type, Args...>)
        unsigned int add_box(Args&... args) {
            box_set.emplace_back(args...);
            return box_set.size() - 1;
        }

        const bounding::box_type& get_box(const unsigned int box_index) const {
            return box_set[box_index];
        }

        friend class debug;
};

min_max_vectors compute_bounding_vectors_bounding_set(
    const std::vector<const bounding*>& set, const bvh& bvh);


template<typename T>
requires (std::is_same_v<T, object> || std::is_same_v<T, bounding>)
[[nodiscard]] static const bounding* containing_bounding_template(std::vector<const T*>&& set, bvh& bvh) {

    const std::size_t size = set.size();

    if (size == 0)
        throw std::runtime_error("Error: creating bounding box of empty set\n");
    
    if (size == 1) {
        if constexpr (std::is_same_v<T, bounding>)
            return set[0];
        else {
            const object* obj = set[0];
            const min_max_coord mmc = obj->get_min_max_coord();
            const unsigned int box_index = bvh.add_box(mmc);
            return new bounding({ obj }, box_index);
        }
    }

    /* Computation of the dimensions of the object set */
    min_max_vectors mmv;
    if constexpr (std::is_same_v<T, object>) {
        mmv = compute_bounding_vectors(set);
    }
    else {
        mmv = compute_bounding_vectors_bounding_set(set, bvh);
    }
    const auto& [ min, max ] = mmv;

    static_assert(
           std::is_same_v<bounding::box_type, box>
        || std::is_same_v<bounding::box_type, aabb>
    );

    /* Creation of the bounding object depending on the type of box and type of AABB */

    const rt::vector& corner = min;
    const rt::vector center = (max + min) / 2.0_r;
    const rt::vector dims = max - min;

    if constexpr (std::is_same_v<bounding::box_type, box>) {

        const unsigned int box_index = bvh.add_box(center, dims);
        return new bounding(
            std::forward<std::vector<const T*>>(set),
            box_index
        );

    }
    else if constexpr (std::is_same_v<bounding::box_type, aabb>) {
        using enum aabb::type;

        unsigned int box_index = 0;
        if constexpr (aabb::type_ == Corner)
            box_index = bvh.add_box(corner, dims);
        else if constexpr (aabb::type_ == Center)
            box_index = bvh.add_box(center, dims);

        return new bounding(
            std::forward<std::vector<const T*>>(set),
            box_index
        );
    }
}

/* Returns an AABB containing the bounding boxes bd0 and bd1 */
[[nodiscard]] inline const bounding* containing_bounding_two(const bounding* bd0, const bounding* bd1, bvh& bvh) {
    return containing_bounding_template<bounding>({ bd0, bd1 }, bvh);
}

/* Returns a non-terminal AABB containing the non-terminal AABBs in the children vector */
[[nodiscard]] inline const bounding* containing_bounding_any(std::vector<const bounding*>&& children, bvh& bvh) {
    return containing_bounding_template(std::move(children), bvh);
}

/* Returns an AABB containing the objects whose indices are in the obj vector */
[[nodiscard]] inline const bounding* containing_objects(std::vector<const object*>&& obj, bvh& bvh) {
    return containing_bounding_template(std::move(obj), bvh);
}
