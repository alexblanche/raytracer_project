#include "file_readers/parsers/obj_parser.hpp"

#include "accelerating_structures/clustering.hpp"
#include "file_readers/parsers/mtl_parser.hpp"
#include "file_readers/file.hpp"
#include "auxiliary/utils.hpp"

#include <array>
#include <stack>
#include <stdexcept>
#include <filesystem>

#include <sstream>
#include <limits>
#include <charconv>

#include <algorithm>

static constexpr bool DISPLAY_HIERARCHY = false; // Displays the characteristics of the 
static constexpr bool PRINT_INDEX       = false; // Writes the v, vt, vn vectors in a file for debugging

/* Quad splitting threshold: when the two triangles forming a quad form an angle
superior to a certain amount depending on this constant,
split the quad into two triangles, to solve some visual glitches */
/* The value 1.0E-7 is chosen empirically: it seems to remove all visible glitches by splitting a small number of quads */
/* History: for the stool, 1.0E-6 is sufficient, but leaves visible glitches on the "Porsche 2016" test model. 1.0E-7 removes them. */
static constexpr bool SPLIT_ALL_QUADS = false;
static constexpr real QUAD_SPLIT_THRESHOLD = 1.0e-7_r;

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
    std::vector<rt::vector>&   vertex_set;
    std::vector<rt::vector>&   uv_coord_set;
    std::vector<rt::vector>&   normal_set;
    std::vector<const object*>&      obj_set;
    std::vector<const object*>&      content;
    scene::containers::orientation&  orientation_containers;
};

template<unsigned int size>
using index_array = std::array<int, size>;

template<unsigned int size>
struct index_description {
    index_array<size> v, vt, vn;
};

struct final_v {
    rt::vector
        v  = rt::ZERO,
        vt = rt::ZERO,
        vn = rt::ZERO;
};

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

struct mapping_parameters {
    unsigned int        current_material_index;
    mapping::index_type current_mapping_index;
    texturing           texturing_option;
    normal              normal_option;
};

template<Polygon polygon, subdivision subdiv_option>
constexpr unsigned int size =
      std::is_same_v<polygon, quad> ?          4
    : subdiv_option == subdivision::Disabled ? 3 : 2;

struct counters {
    unsigned int number_of_vertices        = 0;
    unsigned int number_of_texture_coords  = 0;
    unsigned int number_of_normals         = 0;
    unsigned int number_of_triangles       = 0;
    unsigned int number_of_quads           = 0;
    unsigned int number_of_polygons        = 0;

    template<Polygon polygon>
    void increase() {
        if constexpr (std::is_same_v<polygon, triangle>)
            number_of_triangles++;
        else
            number_of_quads++;
            
        number_of_polygons++;
    }
};

struct polygon_manager {
    std::vector<triangle>&   triangle_set;
    std::vector<quad>&       quad_set;
    const model_positioning& positioning;
    sets                     sets;
    counters                 counters;
    mapping_parameters       mapping_params;
    
    /* Min-max dimensions */
    struct min_max_dims {
        rt::vector min = min_max_coord::min_empty;
        rt::vector max = min_max_coord::max_empty;
    };
    min_max_dims             min_max;

    const bool               bounding_enabled;

    public:
        // nb should be 3 (for triangle), 4 (for quad) or 5 (for polygon with 5 vertices or more)
        void add_geometry(std::istringstream& stream, const int nb,
            index_description<5>& id) {

            if (nb < 3 || nb > 5)
                throw std::runtime_error("add_geometry: Incorrect parameter nb = " + std::to_string(nb));

            auto& [ v, vt, vn ] = id;
            const auto& [ number_of_vertices, number_of_texture_coords, number_of_normals, _, _, _ ] = counters;

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
                    add_triangle({ .v = { v1, v2, v3 }, .vt = { vt1, vt2, vt3 }, .vn = { vn1, vn2, vn3 } });
                    break;
                case 4:
                    /* Sometimes quads are made up of 4 non-coplanar vertices
                        When it is the case, we split the quad in two triangles */
                    add_quad_check_split({ .v = { v1, v2, v3, v4 }, .vt = { vt1, vt2, vt3, vt4 }, .vn = { vn1, vn2, vn3, vn4 } });
                    break;
                default:
                    /* Polygons with more than 4 sides */
                    add_subdivided_polygon(stream,
                        { .v = { v1, v2, v3, v4, v5 }, .vt = { vt1, vt2, vt3, vt4, vt5 }, .vn = { vn1, vn2, vn3, vn4, vn5 } }
                    );
                    break;
            }
        }

    private:
        template<Polygon polygon, unsigned int size>
        requires (size_fits_type<polygon, size>)
        inline void new_orientation_info(
            const rt::vector& v_1, const rt::vector& v_2, // triangle/quad's v1, v2 = p1-p0, p2-p0
            const index_array<size>& vt, const rt::vector& final_vt = rt::vector()) {

            auto& [ _, uv_coord_set, _, _, _, orientation_containers ] = sets;

            constexpr unsigned int uvc_size = (size >= 3) ? size : 3;
            std::array<uvcoord, uvc_size> uv;

            for (int i = 0; unsigned int vti : vt) {
                uv[i++] = {
                    .u = uv_coord_set[vti].x,
                    .v = uv_coord_set[vti].y
                };
            }

            if constexpr (size == 2)
                uv[2] = { final_vt.x, final_vt.y };

            const auto& [ _, current_mapping_index, _, _ ] = mapping_params;
            if constexpr (std::is_same_v<polygon, triangle>)
            
                orientation_containers.triangle_orientation_set.emplace_back(
                    current_mapping_index, uv, v_1, v_2
                );
            else
                orientation_containers.quad_orientation_set.emplace_back(
                    current_mapping_index, uv, v_1, v_2
                );
        }

        template<Polygon polygon, int size>
        requires (size_fits_type<polygon, size>)
        inline const polygon* build_polygon(
            std::vector<polygon>& polygon_set,
            const unsigned int orientation_info_index,
            const index_description<size>& id,
            const final_v& final = final_v()) {

            const auto& [ v, _, vn ] = id;
            const auto& [ vertex_set, _, normal_set, _, _, _ ] = sets;

            const auto& [ ...vi ]     = v;
            const auto  [ ...vert_i ] = positioning.is_not_null() ?
                std::array { positioning.position(vertex_set[vi])... }
                : std::array { vertex_set[vi]... };
            
            if (polygon_set.size() == polygon_set.capacity()) {
                
                const auto error_string = [&polygon_set] () {
                    const std::string type = std::is_same_v<polygon, triangle> ? "triangle" : "quad";
                    return "obj_parser Error: " + type + " set capacity reached ("
                    + std::to_string(polygon_set.size()) + " / " + std::to_string(polygon_set.capacity()) + ")";
                };
                throw std::runtime_error(error_string());
            }

            const auto& [ current_material_index, _, _, normal_option] = mapping_params;

            if (normal_option == normal::Enabled) {

                const auto& [ ...vni ]    = vn;
                const auto& [ ...norm_i ] = std::array { normal_set[vni]... };
                static_assert(TODO_ROTATION_OF_MODELS); // rotate the normals as well

                if constexpr (size >= 3)
                    polygon_set.emplace_back(vert_i..., norm_i...,
                        current_material_index, orientation_info_index);
                else
                    polygon_set.emplace_back(vert_i..., final.v, norm_i..., final.vn,
                        current_material_index, orientation_info_index);
            }
            else {
                if constexpr (size >= 3)
                    polygon_set.emplace_back(vert_i...,
                        current_material_index, orientation_info_index);
                else
                    polygon_set.emplace_back(vert_i..., final.v,
                        current_material_index, orientation_info_index);
            }

            return &polygon_set.back();
        }

        template<
            Polygon polygon,
            subdivision subdiv_option = subdivision::Disabled,
            unsigned int size = size<polygon, subdiv_option>
        >
        inline void add_polygon(index_description<size>&& id, const final_v& final = final_v()) {

            const auto& [ _, current_mapping_index, texturing_option, _ ] = mapping_params;

            const bool texturing_enabled = (texturing_option == texturing::Enabled) && (current_mapping_index != EMPTY_INDEX);

            auto& [ _, _, _, object_set, content, orientation_containers ] = sets;

            unsigned int orientation_info_index;
            if (not texturing_enabled)
                orientation_info_index = EMPTY_INDEX;
            else if constexpr (std::is_same_v<polygon, triangle>)
                orientation_info_index = orientation_containers.triangle_orientation_set.size();
            else
                orientation_info_index = orientation_containers.quad_orientation_set.size();
                
            const polygon* poly;
            if constexpr (std::is_same_v<polygon, triangle>)
                poly = build_polygon<triangle, size>(
                    triangle_set,
                    orientation_info_index,
                    id, final
                );
            else
                poly = build_polygon<quad, size>(
                    quad_set,
                    orientation_info_index,
                    id, final
                );

            const auto& [ v_1, v_2 ] = poly->get_v1_v2();
            const auto& [ _, vt, _ ] = id;

            if (texturing_enabled)
                new_orientation_info<polygon, size>(v_1, v_2, vt, final.vt);

            object_set.push_back(poly);
            if (bounding_enabled) [[likely]]
                content.push_back(poly);

            counters.increase<polygon>();
        }

        /* Auxiliary function that subdivides a polygon with more than 5 sides into triangles, and adds all of them */
        void add_subdivided_polygon(std::istringstream& stream, index_description<5>&& id) {

            const auto& [ _, _, texturing_option, normal_option ] = mapping_params;

            const bool normal_enabled = normal_option    == normal::Enabled;
            const bool apply_texture  = texturing_option == texturing::Enabled;

            // const std::string error_message = "Error in parsing of polygons of at least 5 sides ("
            //     + std::string(normal_enabled ? "with" : "without") + "normal)\n";

            auto& [ vertex_set, uv_coord_set, normal_set, _, _, _ ] = sets;

            auto& [ v, vt, vn ] = id;

            std::stack<int> v_stack, vt_stack, vn_stack;
            v_stack.push_range(v);
            vt_stack.push_range(vt);
            if (normal_enabled)
                vn_stack.push_range(vn);

            final_v final;
            for (int i = 0; i < 5; i++) {
                final.v  += vertex_set[v[i]];
                final.vt += uv_coord_set[vt[i]];
                if (normal_enabled)
                    final.vn += normal_set[vn[i]];
            }

            unsigned int cpt = 5;

            // Reading triplets until the end of the line
            
            unsigned char ch;
            while ((not stream.eof()) && ((ch = stream.peek()) != '\n' && ch != '\r')) {
                
                int vi = 0, vti = 0, vni = 0;
                stream >> vi;
                if (stream.peek() == '/') {
                    stream.get(); // '/'
                    if (apply_texture)
                        stream >> vti;
                    char d; // receives '/'
                    if (normal_enabled)
                        stream >> d >> vni;
                }

                correct(vi, vertex_set.size() - 1);
                v_stack.push(vi);
                if (apply_texture) {
                    correct(vti, uv_coord_set.size() - 1);
                    vt_stack.push(vti);
                }

                final.v += vertex_set[vi];
                if (apply_texture)
                    final.vt += uv_coord_set[vti];
                
                if (normal_enabled) {
                    correct(vni, normal_set.size() - 1);
                    vn_stack.push(vni);
                    final.vn += normal_set[vni];
                }

                cpt++;
            }

            // New central vertex
            final.v /= cpt;
            if (apply_texture)
                final.vt /= cpt;
            if (normal_enabled)
                final.vn /= cpt;

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
                if (normal_enabled)
                    vn_stack.pop();

                const int vj  = v_stack.top();
                const int vtj = apply_texture  ? vt_stack.top() : 0;
                const int vnj = normal_enabled ? vn_stack.top() : 0;
                
                add_polygon<triangle, subdivision::Enabled>(
                    { .v = { vj, vi }, .vt = { vtj, vti }, .vn = { vnj, vni } },
                    final);
            }
            
            // Adding the last triangle
            add_polygon<triangle, subdivision::Enabled>(
                { .v = { last_v, v[0] }, .vt = { last_vt, vt[0] }, .vn = { last_vn, vn[0] } },
                final);
        }

        inline void add_triangle(index_description<3>&& id) {
            
            constexpr unsigned int size = 3;
            add_polygon<triangle, subdivision::Disabled, size>(std::move(id));
        }

        inline void add_quad(index_description<4>&& id) {

            constexpr unsigned int size = 4;
            add_polygon<quad, subdivision::Disabled, size>(std::move(id));
        }

        inline void add_quad_check_split(index_description<4>&& id) {

            const auto& [ v, vt, vn ] = id;
            const auto [ n12, n23 ] = compute_normals(sets.vertex_set, v);
            const bool is_split_quad = SPLIT_ALL_QUADS || ((n12 - n23).normsq() > QUAD_SPLIT_THRESHOLD);

            if (not is_split_quad) {
                add_quad(std::move(id));
            }
            else {
                const auto& [ v1,  v2,  v3,  v4  ] = v;
                const auto& [ vt1, vt2, vt3, vt4 ] = vt;
                const auto& [ vn1, vn2, vn3, vn4 ] = vn;
                add_triangle({ .v = { v1, v2, v3 }, .vt = { vt1, vt2, vt3 }, .vn = { vn1, vn2, vn3 } });
                add_triangle({ .v = { v1, v3, v4 }, .vt = { vt1, vt3, vt4 }, .vn = { vn1, vn3, vn4 } });
            }
        }
};

static void parse_face_declaration(std::istringstream& stream, polygon_manager& poly_manager) {

    static std::string line;
    static std::istringstream line_stream;

    std::getline(stream, line, '\n');
    line_stream = std::istringstream(std::move(line));

    index_description<5> id;
    auto& [ v, vt, vn ] = id;

    line_stream >> v[0];

    enum class face_type {
        Full, NoTexture, NoNormal, NoTextureNoNormal
    };
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
    
    auto& [ _, _, texturing_option, normal_option ] = poly_manager.mapping_params;
    texturing_option = type == NoTexture || type == NoTextureNoNormal ?
          texturing::Disabled
        : texturing::Enabled;

    normal_option = type == NoNormal || type == NoTextureNoNormal ?
          normal::Disabled
        : normal::Enabled;

    poly_manager.add_geometry(line_stream, nb, id);
}

static void parse_vertex_declaration(const std::string& arg, std::istringstream& stream, polygon_manager& poly_manager) {

    static std::string line;

    std::getline(stream, line);
    const char * buffer = line.data() + 1;

    enum class vtype {
        V, VT, VN
    };
    using enum vtype;
    const vtype type = (arg == "v") ? V : (arg == "vt") ? VT : VN;

    auto& [ vertex_set, uv_coord_set, normal_set, _, _, _ ] = poly_manager.sets;
    auto& [ number_of_vertices, number_of_texture_coords, number_of_normals, _, _, _ ] = poly_manager.counters;

    switch (type) {
        case V:
        case VN: {
            /* Vertex definition */

            const auto [ x, y, z ] = parse<double, 3>(buffer, line.size());

            if (type == V) {
                vertex_set.emplace_back(x, y, z);
                number_of_vertices++;

                /* Updating max dimensions */
                auto& [ min, max ] = poly_manager.min_max;
                min = rt::min(min, vertex_set.back());
                max = rt::max(max, vertex_set.back());
            }
            else {
                normal_set.emplace_back(x, y, z);
                number_of_normals++;
            }
        }
        break;
        
        case VT: {
            /* Texture UV-coordinates definition */

            const auto [ u, v ] = parse<double, 2>(buffer, line.size());

            if (is_between_zero_and_one(u) && is_between_zero_and_one(v)) [[likely]] {
                
                uv_coord_set.emplace_back(u, v, 0);
            }
            else {
                const real nu = (u >= 0) ? 1.0_r : ((u <= (-1.0_r)) ? 0.0_r : 1.0_r + u);
                const real nv = (v >= 0) ? 1.0_r : ((v <= (-1.0_r)) ? 0.0_r : 1.0_r + v);
                uv_coord_set.emplace_back(nu, nv, 0);
            }
            number_of_texture_coords++;
            break;
        }

        default: throw;
    }
}

static void print_result(const polygon_manager& poly_manager) {

    const auto& [ _, _, positioning, sets, counters, _, min_max, _ ] = poly_manager;

    const auto& [ number_of_vertices, _, _, number_of_triangles, number_of_quads, number_of_polygons ] = counters;
    printf("%u vertices, %u polygons (%u triangles, %u quads)\n",
        number_of_vertices, number_of_polygons, number_of_triangles, number_of_quads);
    
    const auto& [ min, max ] = min_max;
    printf("Dimensions: (x: [%lf; %lf]; y: [%lf; %lf]; z: [%lf; %lf])\n",
        min.x, max.x, min.y, max.y, min.z, max.z);
    
    if (positioning.is_not_null()) {

        const auto& [ shift, scale ] = positioning.get_content();
        const rt::vector scaled_min = fma(min, scale, shift);
        const rt::vector scaled_max = fma(max, scale, shift);
        printf("Rescaled/shifted dimensions: (x: [%lf; %lf]; y: [%lf; %lf]; z: [%lf; %lf])\n",
            scaled_min.x, scaled_max.x, scaled_min.y, scaled_max.y, scaled_min.z, scaled_max.z);
    }

    if constexpr (PRINT_INDEX) {

        const auto& [ vertex_set, uv_coord_set, normal_set, _, _, _ ] = sets;

        file f("index.txt", "w");

        for (unsigned int i = 0; const rt::vector& v : vertex_set)
            f.printf("v %u (%lf, %lf, %lf)\n",  i++, v.x, v.y, v.z);

        for (unsigned int i = 0; const rt::vector& vt : uv_coord_set)
            f.printf("vt %u (%lf, %lf)\n",      i++, vt.x, vt.y);

        for (unsigned int i = 0; const rt::vector& vn : normal_set)
            f.printf("vn %u (%lf, %lf, %lf)\n", i++, vn.x, vn.y, vn.z);
    }
}

static std::array<std::vector<rt::vector>, 3> build_sets(const unsigned int expected_size) {
    
    /* All indices start at 1, so for simplicity we add an unused first vector */

    std::array<std::vector<rt::vector>, 3> out;
    for (auto& v : out) {
        v.reserve(expected_size);
        v.emplace_back();
    }
    return out;
}

/* Parses .obj file file_name. Triangles and quads are added to obj_set,
with material indices (defined with the keyword usemtl) found in material_names

    - Object names (o), polygon groups (g), smooth shading (s), lines (l) are ignored.
    - The object is scaled with the factor scale, and shifted by the vector shift (members of positioning)
    - If bounding_enabled, a bounding containing the whole object is placed in output_bd.
        It contains a hierarchy of bounding boxes, such that the terminal ones contain at most
        polygons_per_bounding polygons.
*/

exit_status parse_obj_file(const std::string& file_name,
    const std::optional<mapping::index_type> default_mapping_index,
    containers& containers, const model_positioning& positioning,
    const bool bounding_enabled, const unsigned int polygons_per_bounding, const bounding*& output_bd, // Encapsulate
    const std::optional<real> gamma) {

    printf("Parsing obj file... ");
    fflush(stdout);

    /* Extraction of the path to the .obj file, to be appended to relative paths of mtl and texture files */
    const std::filesystem::path path = std::filesystem::path(file_name).parent_path();

    /* Material -> mapping association table */
    material_mapping_map mt_assoc;

    auto& [
        object_set,
        _,
        object_containers,
        material_wrapper_set,
        _, _, _,
        orientation_containers
    ]
    = containers;

    /* Storage */
    const unsigned int expected_size = 2 * object_containers.triangle_set.capacity() / 3;
    auto [ vertex_set, uv_coord_set, normal_set ] = build_sets(expected_size);

    /* Bounding containers
        content will contain the polygons of a group before being placed in a bounding,
        which will be added to the children vector
        At the end, a bounding containing all the ones in bounding is placed in output_bounding */
    std::vector<const object*> content;
    std::vector<const bounding*> children;

    polygon_manager poly_manager = {
        .triangle_set = object_containers.triangle_set,
        .quad_set     = object_containers.quad_set,
        .positioning  = positioning,
        .sets         = {
            vertex_set, uv_coord_set, normal_set, object_set, content, orientation_containers
        },
        .mapping_params = {
            .current_material_index = 0,
            .current_mapping_index  = default_mapping_index.value_or(EMPTY_INDEX),
            .texturing_option       = texturing::Disabled,
            .normal_option          = normal::Disabled
        },
        .bounding_enabled = bounding_enabled
    };

    try {

        file f(file_name, "rb");
        const std::vector<unsigned char> buffer = f.extract();
        f.close();

        std::istringstream stream(reinterpret_cast<const char*>(buffer.data()));
        std::string arg;

        /* Parsing loop */
        while (stream >> arg) {

            if (arg == "f") {
                parse_face_declaration(stream, poly_manager);
            }

            else if (arg.starts_with("v")) {
                parse_vertex_declaration(arg, stream, poly_manager);
            }

            else if (arg == "usemtl") {

                /* Using a new material */
                std::string m_name;
                stream >> m_name;

                /* Looking up the material name in the vector of already declared material names */
                const std::optional<unsigned int> vindex = wrapper<material>::find_element(material_wrapper_set, m_name);
                throw_if_nullopt(vindex, "(material reading)");
                
                auto& [ current_material_index, current_mapping_index, _, _ ] = poly_manager.mapping_params;

                current_material_index = vindex.value();

                /* Checking if a mapping was associated with the material by an mtl file */
                current_mapping_index = (mt_assoc.contains(current_material_index)) ?
                      mt_assoc[current_material_index]
                    : default_mapping_index.value_or(EMPTY_INDEX);
            }

            else if (arg == "mtllib") {

                std::string mtl_file_name;
                stream >> mtl_file_name;

                const exit_status mtl_parsing_successful =
                    parse_mtl_file(path, mtl_file_name, material_wrapper_set,
                        containers, mt_assoc, gamma);
                throw_if_failure(mtl_parsing_successful, "(mtl file loading)");
            }

            /* Commented line, or ignored command */
            else if (arg.starts_with("#") || (arg == "s" || arg == "l" || arg == "g" || arg == "o" || arg == "vp")) {

                stream.ignore(MAX_LINE_SIZE, '\n');
            }

            /* New group definition
                If bounding_enabled is true, the current content vector of polygons
                is placed in a box that is added to the children vector */
            else if ((arg == "o" || arg == "g") && not content.empty() && bounding_enabled) {

                // Create a bounding hierarchy containing all the nodes
                /* Heuristic: each group is a depth 1 node in the global bounding box hierarchy */
                const bounding* bd = create_bounding_hierarchy(std::move(content), polygons_per_bounding);
                if constexpr (DISPLAY_HIERARCHY)
                    display_hierarchy_properties(bd);
                children.push_back(bd);
                content.clear();
            }
        }
        
        if (bounding_enabled) [[likely]] {
            /* Placing the last group into a bounding */
            const bounding* bd = create_bounding_hierarchy(std::move(content), polygons_per_bounding);
            if constexpr (DISPLAY_HIERARCHY)
                display_hierarchy_properties(bd);
            children.push_back(bd);

            /* Computing the final bounding */
            output_bd = (children.size() == 1) ?
                  children[0]
                : create_hierarchy_from_boundings(std::move(children));
        }

        printf("\r%s successfully loaded:\n", file_name.c_str());
        print_result(poly_manager);

        return exit_status::Success;
    }
    catch (const std::exception& e) {
        printf("Parsing error in file %s ", file_name.c_str());
        printf("%s\n", e.what());
        return exit_status::Failure;
    }
}