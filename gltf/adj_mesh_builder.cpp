

#include "adj_mesh_builder.hpp"

#include <gltf/misc/gl_vao_utils.hpp>

#include <map>

namespace
{
    struct face
    {
        glm::vec3 v0, v1;
    };

    bool operator==(const face& l, const face& r)
    {
        return (l.v0 == r.v0 && l.v1 == r.v1) || (l.v0 == r.v1 && l.v1 == r.v0);
    }

    template<typename IntType>
    struct triangle
    {
        IntType i0, i1, i2;
        glm::vec3 v0, v1, v2;
    };


    template<typename IntType>
    std::vector<std::array<IntType, 6>> parse_triangles(const void* arr, size_t bytes_size, const glm::vec3* vert_arr, size_t vert_arr_size)
    {
        using triangle = triangle<IntType>;
        using triangles_list = std::vector<triangle>;
        using faces_list = std::vector<std::pair<face, triangles_list>>;

        const uint64_t arr_size = bytes_size / sizeof(IntType);
        assert(arr_size % 3 == 0);
        const IntType* int_arr = reinterpret_cast<const IntType*>(arr);

        triangles_list triangles;
        triangles.reserve(arr_size / 3);
        faces_list faces;
        faces.reserve(triangles.size() * 3);

        const auto insert_face_triangle = [&faces](const face& f, const triangle& t) {
            auto f_it = std::find_if(faces.begin(), faces.end(), [&f](const std::pair<face, triangles_list>& p) {
                return f == p.first;
            });

            if (f_it == faces.end()) {
                faces.emplace_back(std::make_pair(f, triangles_list{t}));
            } else {
                auto& [face, tri_list] = *f_it;
                assert(tri_list.size() < 2);
                tri_list.emplace_back(t);
            }
        };

        const auto find_not_adj_index = [](const triangle& l, const triangle& r) {
            auto lf0 = face{l.v0, l.v1};
            auto lf1 = face{l.v0, l.v2};
            auto lf2 = face{l.v1, l.v2};

            auto rf0 = face{r.v0, r.v1};
            auto rf1 = face{r.v0, r.v2};
            auto rf2 = face{r.v1, r.v2};

            if (lf0 == rf0 || lf1 == rf0 || lf2 == rf0) {
                return r.i2;
            } else if (lf0 == rf1 || lf1 == rf1 || lf2 == rf1) {
                return r.i1;
            } else if (lf0 == rf2 || lf1 == rf2 || lf2 == rf2) {
                return r.i0;
            }

            assert(false);
        };

        for (size_t i = 0; i < arr_size; i += 3) {
            const IntType i0 = int_arr[i];
            const IntType i1 = int_arr[i + 1];
            const IntType i2 = int_arr[i + 2];

            const glm::vec3 v0 = vert_arr[i0];
            const glm::vec3 v1 = vert_arr[i1];
            const glm::vec3 v2 = vert_arr[i2];

            triangle t{i0, i1, i2, v0, v1, v2};

            triangles.emplace_back(t);

            insert_face_triangle(face{v0, v1}, t);
            insert_face_triangle(face{v0, v2}, t);
            insert_face_triangle(face{v1, v2}, t);
        }

        std::vector<std::array<IntType, 6>> ret;
        ret.reserve(triangles.size());

        auto find_face = [&faces](glm::vec3 v0, glm::vec3 v1) {
            return std::find_if(faces.begin(), faces.end(), [&v0, &v1](const std::pair<face, triangles_list>& p) {
                return (v0 == p.first.v0 && v1 == p.first.v1) || (v0 == p.first.v1 && v1 == p.first.v0);
            });
        };

        for (const auto& t : triangles) {
            size_t i;

            std::array<IntType, 6> indices;

            indices[0] = t.i0;
            indices[2] = t.i1;
            indices[4] = t.i2;

            const auto& [face1, l1] = *find_face(t.v0, t.v1);
            const auto& [face3, l3] = *find_face(t.v0, t.v2);
            const auto& [face5, l5] = *find_face(t.v1, t.v2);

            indices[1] = find_not_adj_index(l1[0], l1[1]);
            indices[3] = find_not_adj_index(l3[0], l3[1]);
            indices[5] = find_not_adj_index(l5[0], l5[1]);

            ret.emplace_back(indices);
        }

        return ret;
    }
}


void gltf::adj_mesh_builder::make_mesh(const gltf::mesh& mesh, gl::scene::scene& scene)
{
    for (const auto& subset : mesh.get_geom_subsets()) {
        make_subset(scene, subset);
        auto& s = const_cast<gltf::mesh::geom_subset&>(subset);
        assert(s.topo == gltf::mesh::topo::triangles);
        s.topo = gltf::mesh::topo::triangles_adj;
    }
}


void gltf::adj_mesh_builder::make_subset(gl::scene::scene& gl_scene, gltf::mesh::geom_subset geom_subset)
{
    assert(geom_subset.indices.d_type == gltf::data_storage::type::scalar);

    assert(geom_subset.indices.c_type == gltf::data_storage::component_type::u32 ||
           geom_subset.indices.c_type == gltf::data_storage::component_type::u16 ||
           geom_subset.indices.c_type == gltf::data_storage::component_type::u8);

    auto& vao = gl_scene.vertex_sources.emplace_back();

    if (!geom_subset.positions.data.empty()) {
        utils::fill_vao(geom_subset.positions, vao, 0);
    }

    if (!geom_subset.tex_coords0.data.empty()) {
        utils::fill_vao(geom_subset.tex_coords0, vao, 1);
    }

    if (!geom_subset.normals.data.empty()) {
        utils::fill_vao(geom_subset.normals, vao, 2);
    }

    if (!geom_subset.tangents.data.empty()) {
        utils::fill_vao(geom_subset.tangents, vao, 3);
    }

    if (!geom_subset.joints.data.empty()) {
        utils::fill_vao(geom_subset.joints, vao, 4);
    }

    if (!geom_subset.weights.data.empty()) {
        utils::fill_vao(geom_subset.weights, vao, 5);
    }

    if (!geom_subset.tex_coords1.data.empty()) {
        utils::fill_vao(geom_subset.tex_coords1, vao, 6);
    }

    if (!geom_subset.vertices_colors.data.empty()) {
        utils::fill_vao(geom_subset.vertices_colors, vao, 7);
    }

    uint32_t i_size = 0;
    gl::scene::mesh::indices_type i_type = gl::scene::mesh::indices_type::none;

    if (!geom_subset.indices.data.empty()) {
        gl::buffer<GL_ELEMENT_ARRAY_BUFFER> ebo;

        auto fill_ebo = [&vao, &ebo, &i_size](auto& idata, const data_storage& orig_idata_storage) {
            ebo.fill(idata.data(), idata.size() * sizeof(idata.front()));
            i_size = idata.size() * sizeof(idata.front());
        };

        switch (geom_subset.indices.c_type) {
            case data_storage::component_type::u8:
            {
                const auto res = parse_triangles<uint8_t>(
                    geom_subset.indices.data.data(), geom_subset.indices.data.size(),
                    reinterpret_cast<glm::vec3*>(geom_subset.positions.data.data()),
                    geom_subset.positions.data.size() / sizeof(glm::vec3));
                fill_ebo(res, geom_subset.indices);
                i_type = gl::scene::mesh::indices_type::u8;
            }
                break;
            case data_storage::component_type::u16:
            {
                const auto res = parse_triangles<uint16_t>(
                    geom_subset.indices.data.data(), geom_subset.indices.data.size(),
                    reinterpret_cast<glm::vec3*>(geom_subset.positions.data.data()),
                    geom_subset.positions.data.size() / sizeof(glm::vec3));
                fill_ebo(res, geom_subset.indices);
                i_type = gl::scene::mesh::indices_type::u16;
            }
                break;
            case data_storage::component_type::u32:
            {
                const auto res = parse_triangles<uint32_t>(
                    geom_subset.indices.data.data(), geom_subset.indices.data.size(),
                    reinterpret_cast<glm::vec3*>(geom_subset.positions.data.data()),
                    geom_subset.positions.data.size() / sizeof(glm::vec3));
                fill_ebo(res, geom_subset.indices);
                i_type = gl::scene::mesh::indices_type::u32;
            }
                break;
            default:
                throw std::runtime_error("invalid index type.");
        }

        vao.bind();
        ebo.bind();
        vao.unbind();
        ebo.unbind();

        const auto el_size = utils::get_element_size(geom_subset.indices.c_type);
        assert(geom_subset.indices.d_type == gltf::data_storage::type::scalar);
        i_size = i_size / el_size;
    }

    const auto el_size = utils::get_element_size(geom_subset.positions.c_type);
    const auto el_count = utils::get_elements_count(geom_subset.positions.d_type);
    const auto pos_size = geom_subset.positions.data.size() / (el_size * el_count);

    gl_scene.meshes.emplace_back(gl_scene.vertex_sources.size() - 1, i_type, i_size, pos_size);
    assert(glGetError() == GL_NO_ERROR);
}
