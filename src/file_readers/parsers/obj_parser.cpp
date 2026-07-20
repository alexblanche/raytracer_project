#include "file_readers/parsers/obj_parser.hpp"

#include "accelerating_structures/clustering.hpp"
#include "file_readers/parsers/mtl_parser.hpp"
#include "file_readers/file.hpp"
#include "auxiliary/utils.hpp"

#include <stack>
#include <stdexcept>
#include <filesystem>

#include <sstream>
#include <limits>
#include <charconv>

#include <algorithm>

static constexpr bool DISPLAY_HIERARCHY = false;
static constexpr bool PRINT_INDEX = true;

/* Quad splitting threshold: when the two triangles forming a quad form an angle
superior to a certain amount depending on this constant,
split the quad into two triangles, to solve some visual glitches */
/* The value 1.0E-7 is chosen empirically: it seems to remove all visible glitches by splitting a small number of quads */
/* History: for the stool, 1.0E-6 is sufficient, but leaves visible glitches on the "Porsche 2016" test model. 1.0E-7 removes them. */
static constexpr real QUAD_SPLIT_THRESHOLD = 0.1_r;
    //1.0e-7_r;

// static constexpr unsigned int MAX_NAME_LENGTH     = 64;
// static constexpr unsigned int MAX_FILENAME_LENGTH = 512;

constexpr std::streamsize MAX_LINE_SIZE = std::numeric_limits<std::streamsize>::max();

/**************************************************************************************/

pre_parsing_info_obj pre_parse_obj(const std::string& filename) {

    file f(filename, "rb");
    const std::vector<unsigned char> content = f.extract();
    f.close();

    pre_parsing_info_obj out;
    auto& [ faces, triangles, quads ] = out;

    unsigned int i = 0;
    const unsigned int length = content.size();
    
    auto skip_line = [&] {
        while (i < length && content[i] != '\n')
            i++;
        i++;
    };

    auto count_spaces_in_line = [&] {
        int cpt = 0;
        unsigned char ch;
        while (i < length && ((ch = content[i]) != '\n')) {
            cpt += (ch == ' ');
            i++;
        }
        i++;
        return cpt;
    };

    while (i < length) {
        if (content[i] == 'f') {
            const int nb = count_spaces_in_line();
            switch (nb) {
                case 3:  triangles++;     break;
                case 4:  quads++;         break;
                default: triangles += nb; break;
            }
            faces++;
        }
        else
            skip_line();
    }

    /*
    std::cout << "pre_parse_info_obj:"
        << "\nfaces:     " << faces
        << "\ntriangles: " << triangles
        << "\nquads:     " << quads       << std::endl;
    */
    
    return out;
}


/**************************************************************************************/

struct sets {
    const std::vector<rt::vector>&   vertex_set;
    const std::vector<rt::vector>&   uv_coord_set;
    const std::vector<rt::vector>&   normal_set;
    std::vector<const object*>&      obj_set;
    std::vector<const object*>&      content;
    std::vector<texture_info>&       texture_info_set;
};

template<unsigned int size>
using index_array = std::array<int, size>;

/* When indices are negative, convert them to positive */
static inline int correct(int& v, const int n) {
    return (v += (v < 0 ? n + 1 : 0));
}

static inline std::pair<rt::vector, rt::vector> compute_normals(
    const std::vector<rt::vector>& vertex_set, const index_array<4>& vi) {

    const auto& [ v1, v2, v3, v4 ] = vi;
    const rt::vector dv13 = vertex_set[v3] - vertex_set[v1];
    return {
        ((vertex_set[v2] - vertex_set[v1]) ^ dv13).unit(),
        (dv13 ^ (vertex_set[v4] - vertex_set[v1])).unit()
    };
}

/* Parses and returns size values of type T, and modifies the buffer variable to the next position */
template<typename T, int size>
requires std::is_arithmetic_v<T>
static std::array<T, size> parse(const char *& buffer, unsigned int max_length) {
    std::array<T, size> v;
    const char * pt = buffer;
    for (int i = 0; i < size; i++) {
        const auto [ p, _ ] = std::from_chars(pt, pt + max_length, v[i]);
        pt = p + 1;
    }
    buffer = pt;
    return v;
}

/* Wavefront .obj file parser */
/* Only handles .obj files made up of triangles and quads, for now.
In the future, maybe split polygons with >= 5 sides into triangles */

template<typename type>
concept Polygon = std::is_same_v<type, quad> || std::is_same_v<type, triangle>;

template<typename type, int size>
concept size_fits_type = 
    Polygon<type>
    && (std::is_same_v<type, quad> == (size == 4))
    && size >= 2 && size <= 4;

enum class normal {
    Enabled, Disabled
};

enum class subdivision {
    Enabled, Disabled
};

enum class texturing {
    Enabled, Disabled
};

template<Polygon polygon, subdivision subdiv_option>
constexpr unsigned int size =
      std::is_same_v<polygon, quad> ?          4
    : subdiv_option == subdivision::Disabled ? 3 : 2;

template<unsigned int size>
requires (size >= 2 && size <= 4)
static inline void new_texture_info(std::vector<texture_info>& texture_info_set,
    const std::vector<rt::vector>& uv_coord_set,
    const unsigned int current_texture_index,
    const index_array<size>& vt, const rt::vector& final_vt = rt::vector()) {

    std::array<real, 8> uv {};

    for (int i = 0; unsigned int vti : vt) {
        uv[i++] = uv_coord_set[vti].x;
        uv[i++] = uv_coord_set[vti].y;
    }

    if constexpr (size == 2) {
        uv[4] = final_vt.x;
        uv[5] = final_vt.y;
    }

    texture_info_set.emplace_back(
        current_texture_index,
        std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
        std::move(uv)
    );
}

template<Polygon polygon, int size, normal normal_option = normal::Enabled>
requires size_fits_type<polygon, size>
static inline const polygon* build_polygon(
    std::vector<polygon>& polygon_set,
    const std::vector<rt::vector>& vertex_set, const rt::vector shift, const real scale,
    const std::vector<rt::vector>& normal_set,
    const unsigned int current_material_index, const texturing texturing_option,
    const unsigned int current_texture_info_set_index,
    const index_array<size>& v, const index_array<size>& vn,
    const rt::vector& final_v = rt::vector(), const rt::vector& final_vn = rt::vector()) {

    const unsigned int texture_info_index = texturing_option == texturing::Enabled ?
          current_texture_info_set_index
        : EMPTY_INDEX;

    const auto& [ ...vi ]     = v;
    const auto  [ ...vert_i ] = std::array { fma(vertex_set[vi], scale, shift)... };

    const auto& [ ...vni ]    = vn;
    const auto& [ ...norm_i ] = std::array { normal_set[vni]... };

    
    if (polygon_set.size() == polygon_set.capacity()) {
        const std::string type = (std::is_same_v<polygon, triangle>) ? "triangle" : "quad";
        printf("\nobj_parser Error: %s set capacity reached\n", type.c_str());
        printf("%zu / %zu\n", polygon_set.size(), polygon_set.capacity());
        exit(EXIT_FAILURE);
    }
    // const std::string type = (std::is_same_v<polygon, triangle>) ? "triangle" : "quad";
    // printf("%s %zu / %zu ", type.c_str(), polygon_set.size(), polygon_set.capacity());

    if constexpr (normal_option == normal::Enabled) {
        if constexpr (size >= 3)
            polygon_set.emplace_back(vert_i..., norm_i..., current_material_index, texture_info_index);
        else
            polygon_set.emplace_back(vert_i..., final_v, norm_i..., final_vn, current_material_index, texture_info_index);
    }
    else {
        if constexpr (size >= 3)
            polygon_set.emplace_back(vert_i..., current_material_index, texture_info_index);
        else
            polygon_set.emplace_back(vert_i..., final_v, current_material_index, texture_info_index);
    }

    return &polygon_set.back();
}

template<
    Polygon polygon,
    normal normal_option = normal::Enabled,
    subdivision subdiv_option = subdivision::Disabled,
    unsigned int size = size<polygon, subdiv_option>
>
static inline void add_polygon(const sets& sets, const bool bounding_enabled,
    std::vector<polygon>& polygon_set,
    unsigned int& number_of_polygons, unsigned int& number_of__type__,
    const rt::vector& shift, const real scale,
    index_array<size>&& v, index_array<size>&& vt, index_array<size>&& vn,
    const unsigned int current_texture_index, const unsigned int current_material_index, const texturing texturing_option,
    const rt::vector& final_v = rt::vector(), const rt::vector& final_vt = rt::vector(), const rt::vector& final_vn = rt::vector()) {

    auto& [ vertex_set, uv_coord_set, normal_set, obj_set, content, texture_info_set ] = sets;

    if (texturing_option == texturing::Enabled)
        new_texture_info<size>(texture_info_set, uv_coord_set, current_texture_index, vt, final_vt);

    const polygon* poly = build_polygon<polygon, size, normal_option>(
        polygon_set,
        vertex_set, shift, scale, normal_set,
        current_material_index, texturing_option, texture_info_set.size() - 1,
        v, vn, final_v, final_vn
    );

    obj_set.push_back(poly);
    if (bounding_enabled)
        content.push_back(poly);

    number_of_polygons++;
    number_of__type__++;
}

/* Auxiliary function that subdivides a polygon with more than 5 sides into triangles, and adds all of them */
template<normal normal_option = normal::Enabled>
static void add_subdivided_polygon_template(std::istringstream& stream,
    const sets& sets, const bool bounding_enabled,
    std::vector<triangle>& triangle_set, unsigned int& number_of_polygons, unsigned int& number_of_triangles,
    const rt::vector& shift, const real scale,
    index_array<5>&& v, index_array<5>&& vt, index_array<5>&& vn,
    const unsigned int current_texture_index, const unsigned int current_material_index,
    texturing texturing_option) {

    constexpr bool normal_enabled = normal_option == normal::Enabled;
    const bool apply_texture = texturing_option == texturing::Enabled;

    const std::string error_message = "Error in parsing of polygons of at least 5 sides ("
        + std::string(normal_enabled ? "with" : "without") + "normal)\n";

    auto& [ vertex_set, uv_coord_set, normal_set, obj_set, content, texture_info_set ] = sets;

    std::stack<int> v_stack, vt_stack, vn_stack;
    v_stack.push_range(v);
    vt_stack.push_range(vt);
    if constexpr (normal_enabled)
        vn_stack.push_range(vn);

    rt::vector final_v, final_vt, final_vn;
    for (int i = 0; i < 5; i++) {
        final_v  += vertex_set[v[i]];
        final_vt += uv_coord_set[vt[i]];
        if constexpr (normal_enabled)
            final_vn += normal_set[vn[i]];
    }

    unsigned int cpt = 5;

    // Reading triplets until the end of the line
    
    while ((not stream.eof()) && ((stream.peek()) != '\n')) {

        int vi, vti, vni;
        stream >> vi;
        if (stream.peek() == '/') {
            stream.get(); // '/'
            if (apply_texture)
                stream >> vti;
            char d; // receives '/'
            if constexpr (normal_enabled)
                stream >> d >> vni;
        }

        correct(vi, vertex_set.size() - 1);
        v_stack.push(vi);
        if (apply_texture) {
            correct(vti, uv_coord_set.size() - 1);
            vt_stack.push(vti);
        }

        final_v += vertex_set[vi];
        if (apply_texture)
            final_vt += uv_coord_set[vti];
        
        if constexpr (normal_enabled) {
            correct(vni, normal_set.size() - 1);
            vn_stack.push(vni);
            final_vn += normal_set[vni];
        }

        cpt++;
    }

    // New central vertex
    final_v /= cpt;
    if (apply_texture)
        final_vt /= cpt;
    if constexpr (normal_enabled)
        final_vn /= cpt;

    // Keeping the last vertex in memory to form a triangle with the first vertex
    const int last_v  = v_stack.top();
    const int last_vt = apply_texture  ? vt_stack.top() : 0;
    const int last_vn = normal_enabled ? vn_stack.top() : 0;

    // Adding the new triangles having the new central vertex as a common vertex
    for (unsigned int i = 0; i < cpt - 1; i++) {
        const int vi  = v_stack.top();
        const int vti = apply_texture  ? vt_stack.top() : 0;
        const int vni = normal_enabled ? vn_stack.top() : 0;

        v_stack.pop();
        if (apply_texture)
            vt_stack.pop();
        if constexpr (normal_enabled)
            vn_stack.pop();

        const int vj  = v_stack.top();
        const int vtj = apply_texture  ? vt_stack.top() : 0;
        const int vnj = normal_enabled ? vn_stack.top() : 0;

        add_polygon<triangle, normal_option, subdivision::Enabled>(
            sets, bounding_enabled,
            triangle_set, number_of_polygons, number_of_triangles,
            shift, scale, { vj, vi }, { vtj, vti }, { vnj, vni },
            current_texture_index, current_material_index, texturing_option,
            final_v, final_vt, final_vn
        );
    }
    
    // Adding the last triangle
    add_polygon<triangle, normal_option, subdivision::Enabled>(
        sets, bounding_enabled,
        triangle_set, number_of_polygons, number_of_triangles,
        shift, scale, { last_v, v[0] }, { last_vt, vt[0] }, { last_vn, vn[0] },
        current_texture_index, current_material_index, texturing_option,
        final_v, final_vt, final_vn
    );
}

enum class face_type {
    Full, NoTexture, NoNormal, NoTextureNoNormal
};

bool parse_face(std::istringstream& stream,
    const int nb, const face_type face_type_,
    index_array<5>& v, index_array<5>& vt, index_array<5>& vn) {
    
    std::string line;
    std::getline(stream, line);
    const char* pt = line.data() + 2; // skip "f "

    using enum face_type;
    switch (face_type_) {
        case Full:
            for (int i = 0; i < nb; i++) {
                const auto [ vi, vti, vni ] = parse<int, 3>(pt, line.size());
                v[i]  = vi;
                vt[i] = vti;
                vn[i] = vni;
            }
            break;

        case NoTexture:
            for (int i = 0; i < nb; i++) {
                const auto [ vi, _, vni ] = parse<int, 3>(pt, line.size());
                v[i]  = vi;
                vn[i] = vni;
            }
            break;

        case NoNormal:
            for (int i = 0; i < nb; i++) {
                const auto [ vi, vti ] = parse<int, 2>(pt, line.size());
                v[i]  = vi;
                vt[i] = vti;
            }
            break;
        
        case NoTextureNoNormal:
            for (int i = 0; i < nb; i++) {
                const auto [ p, _ ] = std::from_chars(pt, pt + line.size(), v[i]);
                pt = p + 1;
            }
            break;
    }

    return true;
}

/* Parses .obj file file_name. Triangles and quads are added to obj_set,
with material indices (defined with the keyword usemtl) found in material_names

    - Only one texture is handled.
    - Object names (o), polygon groups (g), smooth shading (s), lines (l) are ignored.
    - The object is scaled with the factor scale, and shifted by the vector shift.
    - If bounding_enabled, a bounding containing the whole object is placed in output_bd.
        It contains a hierarchy of bounding boxes, such that the terminal ones contain at most
        polygons_per_bounding polygons.

    Returns true if the operation was successful
*/

exit_status parse_obj_file(const std::string& file_name,
    const std::optional<unsigned int> default_texture_index,
    containers& containers,
    const real scale, const rt::vector& shift,
    const bool bounding_enabled, const unsigned int polygons_per_bounding,
    const bounding*& output_bd, const std::optional<real> gamma) {

    printf("Parsing obj file... ");
    fflush(stdout);
    
    auto& [
        object_set,
        other_content,

        triangle_set,
        quad_set,
        sphere_set,
        plane_set,
        box_set,
        cylinder_set,

        material_wrapper_set,
        texture_wrapper_set,
        normal_map_wrapper_set,
        texture_info_set
    ]
    = containers;

    // printf("triangle_set: size %zu, capacity %zu\n", triangle_set.size(), triangle_set.capacity());
    // printf("quad_set: size %zu, capacity %zu\n", quad_set.size(), quad_set.capacity());

    /* Extraction of the path to the .obj file, to be appended to relative paths of mtl and texture files */
    const std::filesystem::path path = std::filesystem::path(file_name).parent_path();

    /* Storage */
    /* All indices start at 1, so for simplicity we add an unused first vector */

    const unsigned int expected_size = 2 * triangle_set.capacity() / 3;

    /* Vertices of the object */
    std::vector<rt::vector> vertex_set  (1);

    /* UV-coordinates ("vt"), only the first two attributes (x, y) of the vectors are used */
    std::vector<rt::vector> uv_coord_set(1);

    /* Vertex normals */
    std::vector<rt::vector> normal_set  (1);

    vertex_set  .reserve(expected_size);
    uv_coord_set.reserve(expected_size);
    normal_set  .reserve(expected_size);

    /* Material -> texture association table */
    std::map<unsigned int, unsigned int> mt_assoc;

    unsigned int current_material_index = 0;
    unsigned int current_texture_index = default_texture_index.value_or(EMPTY_INDEX);

    const bool default_texture_provided = default_texture_index.has_value();
    texturing texturing_option = default_texture_provided ? texturing::Enabled : texturing::Disabled;

    /* Counters */
    unsigned int number_of_vertices        = 0;
    unsigned int number_of_triangles       = 0;
    unsigned int number_of_quads           = 0;
    unsigned int number_of_polygons        = 0;
    unsigned int number_of_texture_coords  = 0;
    unsigned int number_of_normals         = 0;

    /* Min-max dimensions */
    rt::vector min = min_max_coord::min_empty;
    rt::vector max = min_max_coord::max_empty;

    /* Bounding containers
        content will contain the polygons of a group before being placed in a bounding,
        which will be added to the children vector
        At the end, a bounding containing all the ones in bounding is placed in output_bounding */
    std::vector<const object*> content;
    std::vector<const bounding*> children;

    sets sets { vertex_set, uv_coord_set, normal_set, object_set, content, texture_info_set };

    auto add_triangle = [&] (index_array<3>&& v, index_array<3>&& vt, index_array<3>&& vn,
        const texturing texturing_option, const normal normal_option = normal::Enabled) {

        constexpr subdivision subdiv_disable = subdivision::Disabled;
        constexpr unsigned int size = 3;

        if (normal_option == normal::Enabled)
            add_polygon<triangle, normal::Enabled, subdiv_disable, size>(
                sets, bounding_enabled,
                triangle_set, number_of_polygons, number_of_triangles,
                shift, scale, std::move(v), std::move(vt), std::move(vn),
                current_texture_index, current_material_index, texturing_option
            );
        else
            add_polygon<triangle, normal::Disabled, subdiv_disable, size>(
                sets, bounding_enabled,
                triangle_set, number_of_polygons, number_of_triangles,
                shift, scale, std::move(v), std::move(vt), std::move(vn),
                current_texture_index, current_material_index, texturing_option
            );
    };

    auto add_quad = [&] (index_array<4>&& v, index_array<4>&& vt, index_array<4>&& vn,
        const texturing texturing_option, const normal normal_option = normal::Enabled) {

        constexpr subdivision subdiv_disable = subdivision::Disabled;
        constexpr unsigned int size = 4;

        if (normal_option == normal::Enabled)
            add_polygon<quad, normal::Enabled, subdiv_disable, size>(
                sets, bounding_enabled,
                quad_set, number_of_polygons, number_of_quads,
                shift, scale, std::move(v), std::move(vt), std::move(vn),
                current_texture_index, current_material_index, texturing_option
            );
        else
            add_polygon<quad, normal::Disabled, subdiv_disable, size>(
                sets, bounding_enabled,
                quad_set, number_of_polygons, number_of_quads,
                shift, scale, std::move(v), std::move(vt), std::move(vn),
                current_texture_index, current_material_index, texturing_option
            );
    };

    auto add_quad_check_split = [&] (const std::vector<rt::vector>& vertex_set,
        index_array<4>&& v, index_array<4>&& vt, index_array<4>&& vn,
        const texturing texturing_option, const normal normal_option = normal::Enabled) {

        const auto [ n12, n23 ] = compute_normals(vertex_set, v);
        const bool is_split_quad = (n12 - n23).normsq() > QUAD_SPLIT_THRESHOLD;

        if (not is_split_quad)
            add_quad(std::move(v), std::move(vt), std::move(vn), texturing_option, normal_option);
        else {
            const auto& [ v1,  v2,  v3,  v4  ] = v;
            const auto& [ vt1, vt2, vt3, vt4 ] = vt;
            const auto& [ vn1, vn2, vn3, vn4 ] = vn;
            add_triangle({ v1, v2, v3 }, { vt1, vt2, vt3 }, { vn1, vn2, vn3 }, texturing_option, normal_option);
            add_triangle({ v1, v3, v4 }, { vt1, vt3, vt4 }, { vn1, vn3, vn4 }, texturing_option, normal_option);
        }
    };

    auto add_subdivided_polygon = [&] (std::istringstream& stream,
        index_array<5>&& v, index_array<5>&& vt, index_array<5>&& vn,
        const texturing texturing_option, const normal normal_option = normal::Enabled) {
        
        if (normal_option == normal::Enabled)
            add_subdivided_polygon_template<normal::Enabled>(stream, sets, bounding_enabled,
                triangle_set, number_of_polygons, number_of_triangles,
                shift, scale, std::move(v), std::move(vt), std::move(vn),
                current_texture_index, current_material_index, texturing_option
            );
        else
            add_subdivided_polygon_template<normal::Disabled>(stream, sets, bounding_enabled,
                triangle_set, number_of_polygons, number_of_triangles,
                shift, scale, std::move(v), std::move(vt), std::move(vn),
                current_texture_index, current_material_index, texturing_option
            );
    };

    // nb should be 3 (for triangle), 4 (for quad) or 5 (for polygon with 5 vertices or more)
    auto add_geometry = [&] (std::istringstream& stream,
        const int nb, const texturing texturing_option, const normal normal_option,
        index_array<5>& v, index_array<5>& vt, index_array<5>& vn) {

        if (nb < 3 || nb > 5)
            throw std::runtime_error((std::string("add_geometry: Incorrect parameter nb = ") + std::to_string(nb)).c_str());

        for (int i = 0; i < nb; i++) {
            correct(v[i],  number_of_vertices);
            correct(vt[i], number_of_texture_coords);
            correct(vn[i], number_of_normals);
        }

        auto& [ v1,  v2,  v3,  v4,  v5  ] = v;
        auto& [ vt1, vt2, vt3, vt4, vt5 ] = vt;
        auto& [ vn1, vn2, vn3, vn4, vn5 ] = vn;

        switch (nb) {
            case 3:
                add_triangle({ v1, v2, v3 }, { vt1, vt2, vt3 }, { vn1, vn2, vn3 },
                    texturing_option, normal_option
                ); break;
            case 4:
                /* Sometimes quads are made up of 4 non-coplanar vertices
                   When it is the case, we split the quad in two triangles */
                add_quad_check_split(vertex_set,
                    { v1, v2, v3, v4 }, { vt1, vt2, vt3, vt4 }, { vn1, vn2, vn3, vn4 },
                    texturing_option, normal_option
                ); break;
            default:
                /* Polygons with more than 4 sides */
                add_subdivided_polygon(stream,
                    { v1, v2, v3, v4, v5 }, { vt1, vt2, vt3, vt4, vt5 }, { vn1, vn2, vn3, vn4, vn5 },
                    texturing_option, normal_option
                ); break;
        }    
    };

    try {

        file f(file_name, "rb");
        const std::vector<unsigned char> buffer = f.extract();
        f.close();

        std::istringstream stream(reinterpret_cast<const char*>(buffer.data()));
        std::istringstream line_stream;
        std::string arg, line;

        /* Parsing loop */
        while (stream >> arg) {
            
            /* New group definition
                If bounding_enabled is true, the current content vector of polygons
                is placed in a box that is added to the children vector */
            if (bounding_enabled && (arg == "o" || arg == "g") && not content.empty()) {

                // Create a bounding hierarchy containing all the nodes
                /* Heuristic: each group is a depth 1 node in the global bounding box hierarchy */
                const bounding* bd = create_bounding_hierarchy(std::move(content), polygons_per_bounding);
                if constexpr (DISPLAY_HIERARCHY)
                    display_hierarchy_properties(bd);
                children.push_back(bd);
                content.clear();
            }

            /* Commented line, or ignored command */
            if (arg.starts_with("#") || (arg == "s" || arg == "l" || arg == "g" || arg == "o" || arg == "vp")) {

                stream.ignore(MAX_LINE_SIZE, '\n');
                continue;
            }

            if (arg == "v") {
                /* Vertex definition */

                std::getline(stream, line);
                const char * buffer = line.data() + 1;
                const auto [ x, y, z ] = parse<double, 3>(buffer, line.size());

                vertex_set.emplace_back(x, y, z);
                number_of_vertices++;

                /* Updating max dimensions */
                min = rt::min(min, vertex_set.back());
                max = rt::max(max, vertex_set.back());

                continue;
            }
            
            if (arg == "vt") {
                /* Texture UV-coordinates definition */

                std::getline(stream, line);
                const char * buffer = line.data() + 1;
                const auto [ u, v ] = parse<double, 2>(buffer, line.size());

                if (is_between_zero_and_one(u) && is_between_zero_and_one(v)) [[likely]] {
                    
                    uv_coord_set.emplace_back(u, v, 0);
                }
                else {
                    // static int cpt_wrong = 0;
                    // cpt_wrong++;
                    // std::cout << "Wrong vt " << cpt_wrong << " / " << uv_coord_set.size() + 1 << std::endl;

                    const real nu = (u >= 0) ? 1.0_r : ((u <= (-1.0_r)) ? 0.0_r : 1.0_r + u);
                    const real nv = (v >= 0) ? 1.0_r : ((v <= (-1.0_r)) ? 0.0_r : 1.0_r + v);
                    uv_coord_set.emplace_back(nu, nv, 0);
                }
                number_of_texture_coords++;

                continue;
            }

            if (arg == "vn") {
                /* Vertex normal definition */
                std::getline(stream, line);
                const char * buffer = line.data() + 1;
                const auto [ x, y, z ] = parse<double, 3>(buffer, line.size());
                
                normal_set.emplace_back(x, y, z);
                number_of_normals++;

                continue;
            }

            if (arg == "usemtl") {
                /* Using a new material */
                std::string m_name;
                stream >> m_name;

                /* Looking up the material name in the vector of already declared material names */
                const std::optional<unsigned int> vindex = wrapper<material>::find_element(material_wrapper_set, m_name);
                throw_if_null(vindex, "(material reading)");
                
                current_material_index = vindex.value();
                if (mt_assoc.count(current_material_index) > 0) {
                    // A texture was associated with the material by an mtl file
                    current_texture_index = mt_assoc[current_material_index];
                    texturing_option = texturing::Enabled;
                }
                else {
                    current_texture_index = default_texture_index.value_or(EMPTY_INDEX);
                    texturing_option = default_texture_provided ? texturing::Enabled : texturing::Disabled;
                }
                
                continue;
            }
            
            if (arg == "mtllib") {
                std::string mtl_file_name;
                stream >> mtl_file_name;

                const exit_status mtl_parsing_successful =
                parse_mtl_file(path, mtl_file_name, material_wrapper_set,
                    texture_wrapper_set, mt_assoc, gamma);
                throw_if_failure(mtl_parsing_successful, "(mtl file loading)");

                continue;
            }

            if (arg == "f") {

                /* Face declaration */

                std::getline(stream, line, '\n');
                line_stream = std::istringstream(std::move(line));

                index_array<5> v, vt, vn;
                line_stream >> v[0];

                using enum face_type;

                face_type type = Full;

                int nb = 1;
                if (line_stream.peek() == ' ') {
                    type = NoTextureNoNormal;
                    while (nb < 5 && line_stream >> v[nb])
                        nb++;
                }
                else {
                    char d1, d2; // receive '/'
                    line_stream.get(); // '/'
                    if (line_stream.peek() == '/') {
                        type = NoTexture;
                        line_stream >> d1 >> vn[0];
                        while (nb < 5 && line_stream >> v[nb] >> d1 >> d2 >> vn[nb])
                            nb++;
                    }
                    else {
                        line_stream >> vt[0];
                        if (line_stream.peek() == '/') {
                            type = Full;
                            line_stream >> d1 >> vn[0];
                            while (nb < 5 && line_stream >> v[nb] >> d1 >> vt[nb] >> d2 >> vn[nb])
                                nb++;
                        }
                        else {
                            type = NoNormal;
                            while (nb < 5 && line_stream >> v[nb] >> d1 >> vt[nb])
                                nb++;
                        }
                    }
                }
                
                const texturing this_texturing_option = type == NoTexture || type == NoTextureNoNormal ?
                      texturing::Disabled
                    : texturing_option;
                
                const normal this_normal_option = type == NoNormal || type == NoTextureNoNormal ?
                      normal::Disabled
                    : normal::Enabled;

                add_geometry(line_stream, nb, this_texturing_option, this_normal_option, v, vt, vn);

                while ((not stream.eof()) && (stream.peek() == 'f') && parse_face(stream, nb, type, v, vt, vn)) {
                    add_geometry(line_stream, nb, this_texturing_option, this_normal_option, v, vt, vn);
                }

                continue;
            }
        }
        
        if (bounding_enabled) {
            /* Placing the last group into a bounding */
            const bounding* bd = create_bounding_hierarchy(std::move(content), polygons_per_bounding);
            if constexpr (DISPLAY_HIERARCHY)
                display_hierarchy_properties(bd);
            children.push_back(bd);

            /* Setting the final bounding */
            if (children.size() == 1)
                output_bd = children[0];
            else
                output_bd = create_hierarchy_from_boundings(std::move(children));
        }

        printf("\r%s successfully loaded:\n", file_name.c_str());
        printf("%u vertices, %u polygons (%u triangles, %u quads)\n",
            number_of_vertices, number_of_polygons, number_of_triangles, number_of_quads);
        printf("Dimensions: (x: [%lf; %lf]; y: [%lf; %lf]; z: [%lf; %lf])\n",
            min.x, max.x, min.y, max.y, min.z, max.z);
        if (scale != 1.0_r || not (shift == ZERO)) {
            const rt::vector scaled_min = fma(min, scale, shift);
            const rt::vector scaled_max = fma(max, scale, shift);
            printf("Rescaled/shifted dimensions: (x: [%lf; %lf]; y: [%lf; %lf]; z: [%lf; %lf])\n",
                scaled_min.x, scaled_max.x, scaled_min.y, scaled_max.y, scaled_min.z, scaled_max.z);
        }

        if constexpr (PRINT_INDEX) {
            file f("index.txt", "w");
            for (unsigned int i = 0; const rt::vector& v : vertex_set) {
                f.printf("v %u (%lf, %lf, %lf)\n", i, v.x, v.y, v.z);
                i++;
            }
            for (unsigned int i = 0; const rt::vector& vt : uv_coord_set) {
                f.printf("vt %u (%lf, %lf)\n", i, vt.x, vt.y);
                i++;
            }
            for (unsigned int i = 0; const rt::vector& vn : normal_set) {
                f.printf("vn %u (%lf, %lf, %lf)\n", i, vn.x, vn.y, vn.z);
                i++;
            }
        }

        return exit_status::Success;
    }
    catch(const std::exception& e) {
        printf("Parsing error in file %s ", file_name.c_str());
        printf("%s\n", e.what());
        return exit_status::Failure;
    }
}