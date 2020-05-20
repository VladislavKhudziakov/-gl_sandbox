

#include "assimp_handlers.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <gl/shaders.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <primitives.hpp>
#include <third/tinygltf/stb_image.h>

namespace
{
    void process_node(
        const aiNode* node,
        const aiScene* scene,
        std::vector<loader::vertex>& vertices,
        std::vector<uint32_t>& indices,
        std::vector<loader::geom_subset>& subsets)
    {
        for (uint64_t i = 0; i < node->mNumMeshes; ++i) {
            const auto& mesh = scene->mMeshes[i];

            const auto base_vertex = vertices.size();

            vertices.reserve(vertices.size() + mesh->mNumVertices);

            for (uint64_t j = 0; j < mesh->mNumVertices; ++j) {
                auto& v = vertices.emplace_back();
                const auto& pos = mesh->mVertices[j];
                const auto& normal = mesh->mNormals[j];
                const auto& tangent = mesh->mTangents[j];
                const auto& uv = mesh->mTextureCoords[0][j];

                v.position = {pos.x, pos.y, pos.z};
                v.normal = {normal.x, normal.y, normal.z};
                v.tangent = {tangent.x, tangent.y, tangent.z};
                v.uv = {uv.x, uv.y};
            }

            auto& subset = subsets.emplace_back();
            subset.base_vertex = base_vertex;
            for (uint64_t k = 0; k < mesh->mNumFaces; ++k) {
                const auto& face = mesh->mFaces[k];
                for (int j = 0; j < face.mNumIndices; ++j) {
                    indices.emplace_back(face.mIndices[j]);
                }
            }
            subset.length = indices.size() - subset.base_vertex;
        }

        for (int l = 0; l < node->mNumChildren; ++l) {
            process_node(node->mChildren[l], scene, vertices, indices, subsets);
        }
    }
} // namespace


loader::mesh_instance loader::load_model(const std::string& file_name)
{
    loader::mesh_instance result{};

    Assimp::Importer importer;
    auto scene = importer.ReadFile(file_name, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs);
    std::vector<vertex> vertices;
    std::vector<uint32_t> indices;
    process_node(scene->mRootNode, scene, vertices, indices, result.subsets);

    {
        gl::buffer<GL_ARRAY_BUFFER> buf;
        buf.fill(vertices.data(), vertices.size() * sizeof(vertices.front()));
        result.vertices.add_vertex_array(buf, 3, sizeof(vertex), 0);
        result.vertices.add_vertex_array(buf, 3, sizeof(vertex), sizeof(glm::vec3));
        result.vertices.add_vertex_array(buf, 3, sizeof(vertex), sizeof(glm::vec3[2]));
        result.vertices.add_vertex_array(buf, 2, sizeof(vertex), sizeof(glm::vec3[3]));
    }

    result.indices.fill(indices.data(), indices.size() * sizeof(indices.front()));
    return result;
}


gl::texture<GL_TEXTURE_2D> loader::load_tex_2d(const std::string& file_name)
{
    int32_t w, h, c;
    auto tex_src = stbi_load(file_name.c_str(), &w, &h, &c, 0);
    gl::texture<GL_TEXTURE_2D> texture;
    texture.fill(reinterpret_cast<const uint8_t*>(tex_src), w, h, c == 3, false);
    stbi_image_free(tex_src);
    return texture;
}

gl::texture<GL_TEXTURE_CUBE_MAP> loader::load_tex_cube(const std::string& file_name, uint32_t w, uint32_t h)
{
    int32_t pw, ph, pc;
    const auto data = stbi_loadf(file_name.c_str(), &pw, &ph, &pc, 0);
    if (data == nullptr) {
        throw std::runtime_error("cannot load image");
    }

    gl::texture<GL_TEXTURE_2D> env_tex;
    env_tex.fill<float>(data, pw, ph, pc);
    gl::texture<GL_TEXTURE_CUBE_MAP> cube_tex{};

    {
        gl::bind_guard g(cube_tex);
        for (int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
    }

    gl::vertex_array_object cube_vao;
    {
        gl::buffer<GL_ARRAY_BUFFER> buf;
        buf.fill(primitives::cube_vertices, sizeof(primitives::cube_vertices));
        cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]));
        cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]), sizeof(float[3]));
        cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]), sizeof(float[3 + 3]));
    }

    glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    glm::mat4 capture_views[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f))};

    constexpr auto vs = R"(#version 410 core
layout (location = 0) in vec3 attr_pos;

out vec3 v_pos;

uniform mat4 u_MVP;

void main()
{
  v_pos = attr_pos;
  gl_Position = u_MVP * vec4(attr_pos, 1.);
}
)";

    constexpr auto fs = R"(#version 410 core
layout (location = 0) out vec4 frag_color;

in vec3 v_pos;

uniform sampler2D s_equirectangular_map;

const vec2 inv_atan = vec2(0.1591, 0.3183);

vec2 sample_spherical_map(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = sample_spherical_map(normalize(v_pos));
    vec3 color = texture(s_equirectangular_map, uv).rgb;

    frag_color = vec4(color, 1.0);
})";

    gl::program p{gl::shader<GL_VERTEX_SHADER>{vs}, gl::shader<GL_FRAGMENT_SHADER>{fs}};

    gl::bind_guard tex_guard(env_tex);
    gl::bind_guard cube_guard(cube_vao);
    gl::bind_guard program_guard(p);
    p.set_uniform("s_equirectangular_map", 0);

    uint32_t fb;

    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);
    for (size_t i = 0; i < 6; ++i) {
        const auto mvp = capture_projection * capture_views[i];
        p.set_uniform("u_MVP", mvp);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cube_tex, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    {
        gl::bind_guard g(cube_tex);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    glDeleteFramebuffers(1, &fb);
    stbi_image_free(data);
    return cube_tex;
}


gl::texture<GL_TEXTURE_CUBE_MAP> loader::load_diff_ibl(const gl::texture<GL_TEXTURE_CUBE_MAP>& env_tex)
{
    constexpr auto vs = R"(#version 410 core
layout (location = 0) in vec3 attr_pos;

out vec3 v_pos;

uniform mat4 u_MVP;

void main()
{
  v_pos = attr_pos;
  gl_Position = u_MVP * vec4(attr_pos, 1.);
}
)";

    constexpr auto fs = R"(#version 410 core
layout (location = 0) out vec4 frag_color;

in vec3 v_pos;

uniform samplerCube s_env_map;

const float PI = 3.14159265359;
const float step = 0.025;

void main()
{
  vec3 irradiance = vec3(0.);
  float samples_count = 0.;

  vec3 N = normalize(v_pos);
  vec3 up = vec3(0.0, 1.0, 0.0);
  vec3 right = cross(up, N);
  up = cross(N, right);

  for (float phi = 0; phi < 2 * PI; phi += step) {
    for (float theta = 0; theta < PI / 2.; theta += step) {
      vec3 tangent_sample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      vec3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * N;
      irradiance += texture(s_env_map, sample_vec).rgb * cos(theta) * sin(theta);
      samples_count += 1;
    }
  }

  frag_color = vec4(PI * irradiance * (1.0 / float(samples_count)), 1.0);
})";

    gl::vertex_array_object cube_vao;
    {
        gl::buffer<GL_ARRAY_BUFFER> buf;
        buf.fill(primitives::cube_vertices, sizeof(primitives::cube_vertices));
        cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]));
        cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]), sizeof(float[3]));
        cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]), sizeof(float[3 + 3]));
    }

    gl::texture<GL_TEXTURE_CUBE_MAP> cube_tex;
    {
        gl::bind_guard g(cube_tex);
        for (int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, 32, 32, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
    }

    gl::program p{gl::shader<GL_VERTEX_SHADER>{vs}, gl::shader<GL_FRAGMENT_SHADER>{fs}};


    gl::bind_guard tex_guard(env_tex);
    gl::bind_guard cube_guard(cube_vao);
    gl::bind_guard program_guard(p);
    p.set_uniform("s_env_map", 0);

    glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    glm::mat4 capture_views[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    uint32_t fb;

    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glViewport(0, 0, 32, 32);

    glEnable(GL_DEPTH_TEST);
    for (size_t i = 0; i < 6; ++i) {
        const auto mvp = capture_projection * capture_views[i];
        p.set_uniform("u_MVP", mvp);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cube_tex, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glDisable(GL_DEPTH_TEST);

    glDeleteFramebuffers(1, &fb);

    return cube_tex;
}
std::pair<gl::texture<GL_TEXTURE_CUBE_MAP>, gl::texture<GL_TEXTURE_2D>>
loader::load_spec_ibl(const gl::texture<GL_TEXTURE_CUBE_MAP>& env_tex)
{
    constexpr auto vs = R"(#version 410 core
layout (location = 0) in vec3 attr_pos;

out vec3 v_pos;

uniform mat4 u_MVP;

void main()
{
  v_pos = attr_pos;
  gl_Position = u_MVP * vec4(attr_pos, 1.);
}
)";

    constexpr auto fs = R"(#version 410 core
layout (location = 0) out vec4 frag_color;

in vec3 v_pos;

uniform float u_roughness;
uniform samplerCube s_env_map;

const float PI = 3.14159265359;
const float step = 0.025;
const uint SAMPLE_COUNT = 1024u;

float radical_inverse_vdc(uint bits)
{
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(uint i, uint N)
{
  return vec2(float(i)/float(N), radical_inverse_vdc(i));
}

vec3 sample_GGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates - halfway vector
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space H vector to world-space sample vector
    vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float distribution_ggx(vec3 N, vec3 H, float roughness)
{
  float a = roughness*roughness;
  float a2 = a*a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH*NdotH;

  float nom   = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return nom / denom;
}

void main()
{
  vec3 N = normalize(v_pos);
  vec3 R = N;
  vec3 V = R;

  float total_weight = 0.0;
  vec3 prefiltered_color = vec3(0.0);

  for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
    vec2 Xi = hammersley(i, SAMPLE_COUNT);
    vec3 H  = sample_GGX(Xi, N, u_roughness);
    vec3 L  = normalize(2.0 * dot(V, H) * H - V);

    float NdotL = max(dot(N, L), 0.0);
    if(NdotL > 0.0) {
      // sample from the environment's mip level based on roughness/pdf
      float D   = distribution_ggx(N, H, u_roughness);
      float NdotH = max(dot(N, H), 0.0);
      float HdotV = max(dot(H, V), 0.0);
      float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

      float resolution = 512.0; // resolution of source cubemap (per face)
      float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
      float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

      float mipLevel = u_roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

      prefiltered_color += textureLod(s_env_map, L, mipLevel).rgb * NdotL;
      total_weight += NdotL;
    }
  }

  prefiltered_color = prefiltered_color / total_weight;

  frag_color = vec4(prefiltered_color, 1.0);
})";

    constexpr auto vs_brdf = R"(#version 410 core
layout (location = 0) in vec3 attr_pos;
layout (location = 1) in vec2 attr_uv;
out vec2 var_uv;

void main()
{
  var_uv = attr_uv;
  gl_Position = vec4(attr_pos, 1.);
})";

    constexpr auto fs_brdf = R"(#version 410 core
layout (location = 0) out vec4 frag_color;

in vec2 var_uv;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits)
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}
// ----------------------------------------------------------------------------
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness*roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space H vector to world-space sample vector
	vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    // note that we use a different k for IBL
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the
        // preferred alignment direction (importance sampling).
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main()
{
  vec2 integratedBRDF = IntegrateBRDF(var_uv.x, var_uv.y);
  frag_color = vec4(integratedBRDF, 0., 1.);
})";

    gl::vertex_array_object cube_vao;
    {
        gl::buffer<GL_ARRAY_BUFFER> buf;
        buf.fill(primitives::cube_vertices, sizeof(primitives::cube_vertices));
        cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]));
        cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]), sizeof(float[3]));
        cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]), sizeof(float[3 + 3]));
    }

    gl::vertex_array_object plane_vao;
    {
        gl::buffer<GL_ARRAY_BUFFER> buf;
        buf.fill(primitives::plane_vertices, sizeof(primitives::plane_vertices));
        plane_vao.add_vertex_array(buf, 3, sizeof(float[3]));
        plane_vao.add_vertex_array(buf, 2, sizeof(float[2]), sizeof(float[3]) * 4);
    }

    gl::buffer<GL_ELEMENT_ARRAY_BUFFER> plane_ebo;
    plane_ebo.fill(primitives::plane_indices, sizeof(primitives::plane_indices));

    gl::texture<GL_TEXTURE_CUBE_MAP> cube_tex;
    {
        gl::bind_guard g(cube_tex);
        for (int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, 128, 128, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    gl::texture<GL_TEXTURE_2D> brdf_tex;
    brdf_tex.fill<float>(nullptr, 512, 512);

    gl::program p{gl::shader<GL_VERTEX_SHADER>{vs}, gl::shader<GL_FRAGMENT_SHADER>{fs}};
    gl::program p2{gl::shader<GL_VERTEX_SHADER>{vs_brdf}, gl::shader<GL_FRAGMENT_SHADER>{fs_brdf}};


    gl::bind_guard tex_guard(env_tex);
    gl::bind_guard cube_guard(cube_vao);
    gl::bind_guard program_guard(p);
    p.set_uniform("s_env_map", 0);

    glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    glm::mat4 capture_views[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};


    uint32_t fb, rbo;
    glGenFramebuffers(1, &fb);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    assert(glGetError() == GL_NO_ERROR);

    size_t max_mip_levels = 5;

    for (size_t curr_mip_lvl = 0; curr_mip_lvl < max_mip_levels; curr_mip_lvl++) {
        auto mip_width = 128 * std::pow(0.5, curr_mip_lvl);
        auto mip_height = 128 * std::pow(0.5, curr_mip_lvl);
        glViewport(0, 0, mip_width, mip_width);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mip_width, mip_height);

        float roughness = (float) curr_mip_lvl / (float) (max_mip_levels - 1);
        p.set_uniform("u_roughness", roughness);

        for (size_t i = 0; i < 6; ++i) {
            const auto mvp = capture_projection * capture_views[i];
            p.set_uniform("u_MVP", mvp);

            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cube_tex, curr_mip_lvl);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    {
        gl::bind_guard g1(plane_vao);
        gl::bind_guard g2(plane_ebo);
        gl::bind_guard g3(p2);

        glViewport(0, 0, 512, 512);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf_tex, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, sizeof(primitives::plane_indices) / sizeof(primitives::plane_indices[0]), GL_UNSIGNED_INT, nullptr);
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glDeleteFramebuffers(1, &fb);
    glDeleteRenderbuffers(1, &rbo);

    assert(glGetError() == GL_NO_ERROR);
    return {std::move(cube_tex), std::move(brdf_tex)};
}
