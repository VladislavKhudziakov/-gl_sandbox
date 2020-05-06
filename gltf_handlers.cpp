

#include <gltf_handlers.hpp>
#include <shaders.hpp>
#include <assimp_handlers.hpp>
#include <primitives.hpp>

#include <glm/gtc/matrix_access.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <third/tinygltf/tiny_gltf.h>

#include <iostream>

namespace gltf
{
  class scene_node;

  struct skin
  {
    std::vector<glm::mat4> inv_bind_poses;
    std::vector<std::shared_ptr<scene_node>> nodes;
  };

  struct anim_keys
  {
    std::vector<glm::vec3> translation;
    std::vector<glm::vec3> scale;
    std::vector<glm::quat> rotation;
  };

  struct animation
  {
    std::vector<std::shared_ptr<scene_node>> nodes;
    std::vector<anim_keys> anim_keys;
  };

  struct scene_node
  {
    uint32_t get_gltf_idx() const
    {
      return m_gltf_idx;
    }

    const glm::mat4& get_world_matrix() const
    {
      return m_world_matrix;
    }

    void set_translation(glm::vec3 t)
    {
      m_translation = t;
    }

    void set_rotation(glm::quat r)
    {
      m_rotation = r;
    }

    void set_scale(glm::vec3 s)
    {
      m_scale = s;
    }

    glm::mat4 calc_TRS() const
    {
      auto t = glm::translate(glm::mat4{1}, m_translation);
      auto r = glm::mat4_cast(m_rotation);
      auto s = glm::scale(glm::mat4{1}, m_scale);
      return t * r * s;
    }

    uint32_t m_gltf_idx;
    glm::mat4 m_world_matrix {1};
    glm::vec3 m_translation {0};
    glm::vec3 m_scale {1};
    glm::quat m_rotation {1.0, 0.0, 0.0, 0.0};

    std::weak_ptr<scene_node> m_parent {};
    std::vector<std::shared_ptr<scene_node>> m_children {};
  };

  using accessor_data = std::tuple<
      const tinygltf::Accessor&,
      const tinygltf::BufferView&,
      const tinygltf::Buffer&,
      const uint8_t*,
      size_t>;

  std::shared_ptr<gltf::scene_node> make_graph(
      const tinygltf::Model &mdl,
      const tinygltf::Node& node,
      const std::shared_ptr<scene_node>& parent)
  {
    auto curr_node = std::make_shared<gltf::scene_node>();
    curr_node->m_parent = parent;

    if (!node.translation.empty()) {
      curr_node->m_translation = {node.translation[0], node.translation[1], node.translation[2]};
    }

    if (!node.rotation.empty()) {
      curr_node->m_rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
    }

    if (!node.scale.empty()) {
      curr_node->m_scale = {node.scale[0], node.scale[1], node.scale[2]};
    }

    curr_node->m_world_matrix = parent->m_world_matrix * curr_node->calc_TRS();

    for (auto node_idx : node.children) {
      curr_node->m_children.emplace_back(gltf::make_graph(mdl, mdl.nodes.at(node_idx), curr_node));
      curr_node->m_children.back()->m_gltf_idx = node_idx;
    }

    return curr_node;
  }

  std::shared_ptr<gltf::scene_node> make_graph(const tinygltf::Model &mdl)
  {
    auto root = std::make_shared<gltf::scene_node>();
    root->m_gltf_idx = -1;

    for (auto node_idx : mdl.scenes.at(0).nodes) {
      root->m_children.emplace_back(gltf::make_graph(mdl, mdl.nodes.at(node_idx), root));
      root->m_children.back()->m_gltf_idx = node_idx;
    }

    auto null_node = std::make_shared<scene_node>();
    root->m_children.emplace_back(null_node);
    null_node->m_parent = root;
    root->m_children.emplace_back(null_node);
    null_node->m_world_matrix = root->m_world_matrix * null_node->calc_TRS();
    null_node->m_gltf_idx = mdl.nodes.size();

    return root;
  }

  std::shared_ptr<gltf::scene_node> find_node_by_index(
      const std::shared_ptr<scene_node>& n, uint32_t idx)
  {
    if (n->m_gltf_idx == idx) {
      return n;
    }

    for (const auto& child : n->m_children) {
      if (const auto& founded_child = find_node_by_index(child, idx); founded_child != nullptr) {
        return founded_child;
      }
    }

    return nullptr;
  }

  accessor_data get_buffer_data(const tinygltf::Model &mdl, uint32_t accessor_idx)
  {
    return {
        mdl.accessors.at(accessor_idx),
        mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView),
        mdl.buffers.at(mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).buffer),
        mdl.buffers.at(mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).buffer).data.data() +
        mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).byteOffset,
        mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).byteLength
    };
  }

  std::vector<gltf::skin> get_skins(const tinygltf::Model& mdl, const std::shared_ptr<gltf::scene_node>& root)
  {
    std::vector<skin> skins;
    skins.reserve(mdl.skins.size());

    for (const auto& skin : mdl.skins) {
      skins.emplace_back();
      skins.back().inv_bind_poses.resize(skin.joints.size());

      auto [ib_accessor, ib_b_view, ib_buf, ib_data, ib_data_size] = get_buffer_data(mdl, skin.inverseBindMatrices);
      std::memcpy(skins.back().inv_bind_poses.data(), ib_data, ib_data_size);

      for (auto joint : skin.joints) {
        auto node = find_node_by_index(root, joint);
        skins.begin()->nodes.emplace_back(node);
      }
    }

    auto& null_skin = skins.emplace_back();
    null_skin.inv_bind_poses.emplace_back(glm::mat4{1});
    const auto& null_node = find_node_by_index(root, mdl.nodes.size());
    null_skin.nodes.emplace_back(null_node);
    return skins;
  }

  std::vector<gltf::animation> get_animations(const tinygltf::Model &mdl, const std::shared_ptr<scene_node>& root)
  {
    std::vector<animation> animations;

    for (const auto& animation : mdl.animations) {
      animations.emplace_back();

      for (const auto& channel : animation.channels) {
        auto& curr_anim = animations.back();
        const auto& sampler = animation.samplers.at(channel.sampler);

        auto node_it = std::find_if(curr_anim.nodes.begin(), curr_anim.nodes.end(), [&](const std::shared_ptr<scene_node>& node) {
          return channel.target_node == node->get_gltf_idx();
        });

        uint32_t idx;

        if (node_it == curr_anim.nodes.end()) {
          curr_anim.nodes.emplace_back(find_node_by_index(root, channel.target_node));
          curr_anim.anim_keys.emplace_back();
          idx = curr_anim.nodes.size() - 1;
        } else {
          idx = node_it - curr_anim.nodes.begin();
        }
        auto [data_accessor, data_b_view, data_buf, data_ptr, data_size] = get_buffer_data(mdl, sampler.output);

        if (channel.target_path == "translation") {
          curr_anim.anim_keys[idx].translation.resize(data_accessor.count);
          std::memcpy(curr_anim.anim_keys[idx].translation.data(), data_ptr, data_size);
        } else if (channel.target_path == "scale") {
          curr_anim.anim_keys[idx].scale.resize(data_accessor.count);
          std::memcpy(curr_anim.anim_keys[idx].scale.data(), data_ptr, data_size);
        } else if (channel.target_path == "rotation") {
          curr_anim.anim_keys[idx].rotation.resize(data_accessor.count);
          std::memcpy(curr_anim.anim_keys[idx].rotation.data(), data_ptr, data_size);
        }
      }
    }

    return animations;
  }

  template <GLenum BufType>
  uint32_t fill_buffer(gl::buffer<BufType>& buf, const tinygltf::Model& mdl, uint32_t accessor)
  {
    auto [d_accessor, d_b_view, d_buf, d_ptr, d_size] = gltf::get_buffer_data(mdl, accessor);
    buf.fill(d_ptr, d_size);
    return d_accessor.count;
  }

  uint32_t fill_vao(gl::vertex_array_object& vao, const tinygltf::Model& mdl, uint32_t accessor)
  {
    auto [d_accessor, d_b_view, d_buf, d_ptr, d_size] = gltf::get_buffer_data(mdl, accessor);
    gl::buffer<GL_ARRAY_BUFFER> buf;
    fill_buffer(buf, mdl, accessor);
    const auto components_count = tinygltf::GetNumComponentsInType(d_accessor.type);
    const auto stride = d_accessor.ByteStride(d_b_view);
    vao.add_vertex_array(buf, components_count, stride, d_accessor.byteOffset, d_accessor.componentType, d_accessor.normalized);
    return d_accessor.count;
  }

  void load_meshes(const tinygltf::Model& mdl, const std::shared_ptr<scene_node>& root /*, const std::vector<animation>& anim */, scene::scene& s)
  {
    auto add_tex = [&s](gl::texture<GL_TEXTURE_2D>& t, scene::material& m, const std::string& s_name) {
      s.textures.emplace_back(std::move(t));
      m.add_texture(s_name, s.textures.size() - 1);
    };

    for (const auto& mesh: mdl.meshes) {
      for (const auto& primitive : mesh.primitives) {
        gl::vertex_array_object vao;
        gl::buffer<GL_ELEMENT_ARRAY_BUFFER> ebo;
        auto indices_size = fill_buffer(ebo, mdl, primitive.indices);

        const auto v_count = fill_vao(vao, mdl, primitive.attributes.at("POSITION"));
        fill_vao(vao, mdl, primitive.attributes.at("TEXCOORD_0"));
        fill_vao(vao, mdl, primitive.attributes.at("NORMAL"));

        if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
          fill_vao(vao, mdl, primitive.attributes.at("TANGENT"));
        }

        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
          fill_vao(vao, mdl, primitive.attributes.at("JOINTS_0"));
        }

        if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
          fill_vao(vao, mdl, primitive.attributes.at("WEIGHTS_0"));
        }

        if (primitive.material >= 0) {
          auto load_img = [&mdl](const auto& tex_info, gl::texture<GL_TEXTURE_2D>& gl_tex) {
            const auto& tex_handler = mdl.textures.at(tex_info.index);
            auto& img_handler = mdl.images.at(tex_handler.source);
            gl_tex.fill<uint8_t>((void*)img_handler.image.data(), img_handler.width, img_handler.height, img_handler.component);
          };

          const auto& mat_handler = mdl.materials.at(primitive.material);

          gl::texture<GL_TEXTURE_2D> normal;
          gl::texture<GL_TEXTURE_2D> albedo;
          gl::texture<GL_TEXTURE_2D> metallic_roughness;

          load_img(mat_handler.normalTexture, normal);
          load_img(mat_handler.pbrMetallicRoughness.baseColorTexture, albedo);
          load_img(mat_handler.pbrMetallicRoughness.metallicRoughnessTexture, metallic_roughness);

          auto& m = s.materials.emplace_back(0);

          add_tex(normal, m, "s_normal");
          add_tex(albedo, m, "s_albedo");
          add_tex(metallic_roughness, m, "s_mrao");

          m.add_texture("s_env", 0);
          m.add_texture("s_ibl_diff", 1);
          m.add_texture("s_ibl_spec", 2);
          m.add_texture("s_brdf", 3);

          m.set_state({
              {true, true, true, true},
              gl::depth_func::leq,
              true,
              true,
              gl::cull_func::back,
          });

          s.vertex_sources.emplace_back(std::move(vao));
          s.index_sources.emplace_back(std::move(ebo));
          s.meshes.emplace_back(s.vertex_sources.size() - 1, s.index_sources.size() - 1, GL_UNSIGNED_INT, indices_size, v_count);
          auto &n = s.nodes.emplace_back();
          n.material_idx = s.materials.size() - 1;
          n.mesh_idx = s.meshes.size() - 1;
        }
      }
    }
  }
}

scene::scene gltf::load_scene(const std::string& file_name, const std::string& env_tex_name)
{
  tinygltf::Model mdl;
  tinygltf::TinyGLTF loader;

  std::string err_msg;
  std::string warn_msg;

  scene::scene s;

  s.camera.fov = glm::radians(60.f);
  s.camera.near = 0.1;
  s.camera.far = 1000;
  s.camera.up = {0, 1, 0};
  s.camera.forward = {0, 0, -1};
  s.camera.position = {0, 0, 3};

  auto env_tex = loader::load_tex_cube(env_tex_name);
  auto ibl_diff = loader::load_diff_ibl(env_tex);
  auto [ibl_spec, ibl_brdf] = loader::load_spec_ibl(env_tex);

  s.textures.emplace_back(std::move(env_tex));
  s.textures.emplace_back(std::move(ibl_diff));
  s.textures.emplace_back(std::move(ibl_spec));
  s.textures.emplace_back(std::move(ibl_brdf));

  gl::framebuffer fb(1600, 1200);
  fb.add_attachment<gl::framebuffer::attachment_target::color1,
      gl::attachment_type::color_rgba8u>();
  float clear_values[] {0, 0, 0, 1};
  fb.get_attachment<gl::framebuffer::attachment_target::color1>().set_clear_values(clear_values);
  fb.add_attachment<gl::framebuffer::attachment_target::depth,
      gl::attachment_type::depth_24f>();

  s.passes.emplace_back(std::move(fb));

  gl::vertex_array_object cube_vao;
  {
    gl::buffer<GL_ARRAY_BUFFER> buf;
    buf.fill(primitives::cube_vertices, sizeof(primitives::cube_vertices));
    cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]));
    cube_vao.add_vertex_array(buf, 2, sizeof(float[3 + 3 + 2]),
                              sizeof(float[3]));
    cube_vao.add_vertex_array(buf, 3, sizeof(float[3 + 3 + 2]),
                              sizeof(float[3 + 3]));
  }

  s.shaders.emplace_back(gl::shader<GL_VERTEX_SHADER>{shaders::pbr_vss}, gl::shader<GL_FRAGMENT_SHADER>{shaders::pbr_fss_ibl});
  s.shaders.emplace_back(gl::shader<GL_VERTEX_SHADER>{shaders::cubemap_bg_vss}, gl::shader<GL_FRAGMENT_SHADER>{shaders::cubemap_bg_fss});

  auto& m = s.materials.emplace_back(1);
  m.add_texture("s_bg", 0);
  m.set_state({
    {true, true, true, true},
    gl::depth_func::leq,
    true,
    true,
    gl::cull_func::off,
  });

  s.vertex_sources.emplace_back(std::move(cube_vao));
  s.meshes.emplace_back(0, -1, -1, -1, 36);
  auto& n = s.nodes.emplace_back();
  n.mesh_idx = 0;
  n.material_idx = 0;

  bool load_success = loader.LoadASCIIFromFile(&mdl, &err_msg, &warn_msg, file_name);

  if (!load_success) {
    throw std::runtime_error(err_msg);
  }

  if (!warn_msg.empty()) {
    std::cerr << warn_msg << std::endl;
  }


  auto root = make_graph(mdl);

  load_meshes(mdl, root, s);

  return s;
}
