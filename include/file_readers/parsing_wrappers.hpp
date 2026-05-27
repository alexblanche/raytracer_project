#pragma once

#include <string>
#include <optional>
#include <span>

#include <vector>
#include <tuple>

/* Struct containing a material, texture or normal map, its name and the index that the objects are going to store */

// Definition of wrappable types
class material;
class texture;
class normal_map;
template<typename T = void> inline const char* type_str() = delete;
template<> inline const char* type_str<material>  () { return "material"  ; }
template<> inline const char* type_str<texture>   () { return "texture"   ; }
template<> inline const char* type_str<normal_map>() { return "normal map"; }

template<typename T>
concept Wrappable = requires(T x) { type_str<T>(); };

template <Wrappable T>
class wrapper {
    public:
        T content;
        std::optional<std::string> name;
        size_t index;

        static inline size_t counter = 0;

        wrapper()
            : index(counter) {
        
            counter++;
        }

        wrapper(T&& t, const std::string& name)
            : content(std::forward<T>(t)), name(name), index(counter) {

            counter++;
        }

        wrapper(const T& t, const std::string& name)
            : content(t), name(name), index(counter) {
        
            counter++;
        }

        wrapper(T&& t)
            : content(std::forward<T>(t)), name(std::nullopt), index(counter) {

            counter++;
        }

        wrapper(wrapper&&)                  = default;
        wrapper& operator=(wrapper&&)       = default;

        wrapper(const wrapper&)             = delete;
        wrapper& operator=(const wrapper&)  = delete;

        /* type = "material", "texture", "normal map" */
        static std::optional<unsigned int> find_element(const std::span<const wrapper> wrapper_set, const char* vname) {

            std::optional<unsigned int> vindex;
        
            for (wrapper const& elt_wrap : wrapper_set) {
                if (elt_wrap.name.has_value() && elt_wrap.name.value() == vname) {
                    vindex = elt_wrap.index;
                    break;
                }
            }

            if (not vindex.has_value()) {
                printf("Error, %s %s not found.\n", type_str<T>(), vname);
                return std::nullopt;
            }
            return vindex;
        }

        static std::vector<T> build_set(const std::span<wrapper> wrapper_set) {
            std::vector<T> set(wrapper::counter);
            for (wrapper& elt_wrap : wrapper_set) {
                set[elt_wrap.index] = std::move(elt_wrap.content);
            }
            return set;
        }
};

template<Wrappable... T>
inline std::tuple<std::vector<T>...> build_sets(const std::span<wrapper<T>>... ws) {
    return std::make_tuple(wrapper<T>::build_set(ws)...);
}

template<Wrappable... T>
inline std::tuple<std::vector<T>...> build_sets(std::vector<wrapper<T>>&... ws) {
    return build_sets(std::span(ws)...);
}