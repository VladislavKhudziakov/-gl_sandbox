

#include "custom_material_builder.hpp"
#include <third/tinygltf/stb_image.h>
namespace
{
    constexpr auto vss = R"(#version 410 core
layout (location = 0) in vec3 attr_pos;
layout (location = 1) in vec2 attr_uv;
layout (location = 2) in vec3 attr_normal;
layout (location = 3) in vec3 attr_tangent;
layout (location = 4) in vec4 attr_bones;
layout (location = 5) in vec4 attr_weights;

out vec2 v_uv;
//out vec3 v_v;
out vec3 v_n;
out vec3 v_t;
//out vec3 v_b;
out vec3 v_view_pos;

uniform mat4 u_MVP;
uniform mat4 u_VIEW;
uniform mat4 u_MODEL;
uniform mat4 u_PROJECTION;

const float PI = 3.14159265;

void main()
{
    mat4 model_transform = u_MODEL;

    vec3 v = attr_pos.xyz;
//    gl_Position = u_MVP * vec4(v, 1.);
   gl_Position = vec4(v, 1.);

//    v_n = vec3(model_transform * vec4(attr_normal, 1.));
//
//    v_t = vec3(model_transform * vec4(attr_tangent, 0.));
//    v_t = normalize(v_t - v_n * max(dot(v_n, v_t), 0.));
//    v_b = cross(v_n, v_t);
//
//    v_v = vec3(u_MODEL * vec4(v, 1.));
//    v_uv = attr_uv;
//    v_view_pos = (u_VIEW)[3].xyz;

    v_n = attr_normal;
    v_t = attr_tangent;
//    v_b = cross(v_n, v_t);

//    v_v = vec3(u_MODEL * vec4(v, 1.));
    v_uv = attr_uv;
    v_view_pos = (u_VIEW)[3].xyz;
}
)";
    constexpr auto fss = R"(#version 410 core
layout (location = 0) out vec4 frag_color;

in vec2 g_uv;
in vec3 g_v;
in vec3 g_n;
in vec3 g_t;
in vec3 g_b;
in vec3 g_view_pos;

vec3 light_pos = vec3(0, 20, 60);
vec3 light_color = vec3(400, 400, 400);

const float TWO_PI = 2 * 3.14159265;

void main()
{
    vec3 ambient = 0.2 * vec3(1., .5, 0.);
    vec3 n = g_n;
    vec3 light_dir = normalize(light_pos - g_v);
    float diff = max(dot(n, light_dir), 0);
    vec3 view_dir = normalize(g_view_pos - g_v);
//    vec3 halfway_dir = view_dir + light_dir;
//    float spec_coeff = pow(max(dot(n, halfway_dir), 0), 40);
    vec3 reflect_dir = reflect(light_dir, n);
    float spec_coeff = pow(max(dot(view_dir, reflect_dir), 0), 12);
    float attenuation = 1 / pow(distance(light_pos, g_v), 2);
    vec3 lightning_color = ((diff + spec_coeff) * attenuation) * light_color;
    vec3 final_color = ambient + pow(lightning_color / (lightning_color + 1), vec3(1 / 2.2));
    frag_color = vec4(final_color, 1.);
//    frag_color = vec4(1., 1., 0., 1.);
}
)";


    constexpr auto gss = R"(#version 410 core
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices=32) out;

in vec2 v_uv[6];
//in vec3 v_v[6];
in vec3 v_n[6];
in vec3 v_t[6];
//in vec3 v_b[6];
in vec3 v_view_pos[6];

out vec2 g_uv;
out vec3 g_v;
out vec3 g_n;
out vec3 g_t;
out vec3 g_b;
out vec3 g_view_pos;

uniform mat4 u_MVP;
uniform mat4 u_VIEW;
uniform mat4 u_MODEL;
uniform mat4 u_PROJ;


vec3 V0;
vec3 V1;
vec3 V2;

float R = 1.;

void prod_vertex(int i)
{
    gl_Position = u_MVP * gl_in[i].gl_Position;
    g_uv = v_uv[i];
    g_v = vec3(u_MODEL * gl_in[i].gl_Position);
    g_n = vec3(u_MODEL * vec4(v_n[i], 0.));
    g_t = vec3(u_MODEL * vec4(v_t[i], 0.));
    g_b = cross(g_n, g_t);
    g_view_pos = v_view_pos[i];
    EmitVertex();
}


void main()
{
    prod_vertex(0);
    prod_vertex(2);
    prod_vertex(4);
    EndPrimitive();
}
)";

}


uint32_t gltf::custom_material_builder::make_material(
    gl::scene::scene& gl_scene,
    const tinygltf::Model& model,
    const gltf::mesh::geom_subset& subset)
{
//    int32_t w, h, c;
//
//    std::unique_ptr<uint8_t, std::function<void(uint8_t*)>> image_data(
//        stbi_load("/Users/vladislavkhudiakov/Downloads/plane/textures/cherries.bmp", &w, &h, &c, 0),
//        [](uint8_t* data) {
//            stbi_image_free(data);
//        });
//
//    assert(image_data != nullptr);

//    gl::texture<GL_TEXTURE_2D> tex;
//    tex.fill(static_cast<const uint8_t*>(image_data.get()), w, h, c);

//    gl_scene.textures.emplace_back(std::move(tex));

    gl_scene.shaders.emplace_back(gl::shader<GL_VERTEX_SHADER>{vss}, gl::shader<GL_FRAGMENT_SHADER>{fss}, gl::shader<GL_GEOMETRY_SHADER>(gss));
    auto& gl_mat = gl_scene.materials.emplace_back(gl_scene.shaders.size() - 1);

    gl_mat.set_state({
     {true, true, true, true},
     true,
     gl::scene::depth_func::leq,
     gl::scene::cull_func::off,
    });

//    gl_mat.add_texture("s_tex", gl_scene.textures.size() - 1);

    return gl_scene.materials.size() - 1;
}
