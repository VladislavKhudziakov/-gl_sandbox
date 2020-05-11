

#include <assimp_handlers.hpp>
#include <gltf/gltf_handlers.hpp>
#include <primitives.hpp>
#include <shaders.hpp>

#include <glm/gtc/matrix_access.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <third/tinygltf/tiny_gltf.h>

#include <iostream>

namespace gltf
{
  class scene_node;

  struct skin_anim
  {
    std::unordered_map<std::string, std::vector<glm::mat4>> animations;
  };

  struct skin
  {
    std::string name;
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
    std::string name;
    std::vector<std::shared_ptr<scene_node>> nodes;
    std::vector<anim_keys> anim_keys;
    size_t keys_size {0};
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

  void update_graph(std::shared_ptr<scene_node>& n)
  {
    const auto local_transform = n->calc_TRS();
    const auto parent_ptr = n->m_parent.lock();

    if (parent_ptr != nullptr) {
      n->m_world_matrix = parent_ptr->m_world_matrix * local_transform;
    } else {
      n->m_world_matrix = local_transform;
    }

    for (auto& child : n->m_children) {
      update_graph(child);
    }
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

  template <typename Callable, typename Container>
  void copy_buffer_data(Callable&& get_data_ptr, Container& container, tinygltf::Model& mdl, uint32_t accessor_idx)
  {
    auto [data_accessor, data_b_view, data_buf, data_ptr, data_size] = get_buffer_data(mdl, accessor_idx);
    container.reserve(data_accessor.count);
    for (size_t i = 0; i < data_accessor.count; ++i) {
      auto& curr_el = container.emplace_back();
      const auto bytes_size = tinygltf::GetComponentSizeInBytes(data_accessor.componentType);
      const auto components_count = tinygltf::GetNumComponentsInType(data_accessor.type);
      assert(sizeof(curr_el) == bytes_size * components_count);
      std::memcpy(get_data_ptr(container.back()), data_ptr + data_accessor.byteOffset, sizeof(curr_el));
      data_ptr += data_b_view.byteStride;
    }
  }

  std::vector<gltf::skin> get_skins(tinygltf::Model& mdl, const std::shared_ptr<gltf::scene_node>& root)
  {
    std::vector<skin> skins;
    skins.reserve(mdl.skins.size());

    for (const auto& skin : mdl.skins) {
      auto& curr_skin = skins.emplace_back();
      curr_skin.name = skin.name;
      curr_skin.nodes.reserve(skin.joints.size());

      copy_buffer_data([](glm::mat4& data) { return glm::value_ptr(data); }, curr_skin.inv_bind_poses, mdl, skin.inverseBindMatrices);

      for (auto joint : skin.joints) {
        curr_skin.nodes.emplace_back(find_node_by_index(root, joint));
      }
    }
    return skins;
  }

  std::vector<gltf::animation> get_animations(tinygltf::Model &mdl, const std::shared_ptr<scene_node>& root)
  {
    std::vector<animation> animations;

    for (const auto& animation : mdl.animations) {
      auto& curr_anim = animations.emplace_back();
      curr_anim.name = animation.name;

      for (const auto& channel : animation.channels) {
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

        if (channel.target_path == "translation") {
          copy_buffer_data([](glm::vec3& data) { return glm::value_ptr(data); }, curr_anim.anim_keys.at(idx).translation, mdl, sampler.output);
        } else if (channel.target_path == "scale") {
          copy_buffer_data([](glm::vec3& data) { return glm::value_ptr(data); }, curr_anim.anim_keys.at(idx).scale, mdl, sampler.output);
        } else if (channel.target_path == "rotation") {
          copy_buffer_data([](glm::quat& data) { return glm::value_ptr(data); }, curr_anim.anim_keys.at(idx).rotation, mdl, sampler.output);
        }

        for (const auto& key : curr_anim.anim_keys) {
          auto curr_size = key.translation.size();
          curr_size = std::max(curr_size, key.scale.size());
          curr_size = std::max(curr_size, key.rotation.size());
          curr_anim.keys_size = std::max(curr_size, curr_anim.keys_size);
        }
      }
    }

    return animations;
  }

  std::vector<skin_anim> calculate_animations(std::vector<gltf::animation>& anims, std::vector<gltf::skin>& skins, std::shared_ptr<scene_node>& root)
  {
    auto get_key_sampler = [](const auto& container, size_t key, auto value) {
      return key < container.size() ? container[key] : value;
    };

    std::vector<skin_anim> skin_anims(skins.size());

    skin_anims.back().animations.emplace("skin", std::vector<glm::mat4>{});

    for (size_t skin = 0; skin < skins.size(); ++skin) {
      auto& skin_impl = skins[skin];
      auto& anims_list = skin_anims[skin].animations.at("skin");
      anims_list.reserve(anims_list.size() + skin_impl.nodes.size());

      for (size_t node = 0; node < skin_impl.nodes.size(); ++node) {
        anims_list.emplace_back(skin_impl.nodes[node]->m_world_matrix * skin_impl.inv_bind_poses[node]);
      }
    }

    std::sort(anims.begin(), anims.end(), [](const animation& l, const animation& r) {
      return l.nodes.size() < r.nodes.size();
    });

    for (const auto& anim : anims) {
      for (auto& curr_skin_anim : skin_anims) {
        curr_skin_anim.animations.emplace(anim.name, std::vector<glm::mat4>{});
      }

      for (size_t key = 0; key < anim.keys_size; ++key) {
        for (size_t node = 0; node < anim.nodes.size(); ++node) {
          auto& node_impl = anim.nodes[node];
          const auto& node_key = anim.anim_keys[node];

          if (key < node_key.translation.size()) {
            node_impl->set_translation(node_key.translation[key]);
          }

          if (key < node_key.scale.size()) {
            node_impl->set_scale(node_key.scale[key]);
          }

          if (key < node_key.rotation.size()) {
            node_impl->set_rotation(node_key.rotation[key]);
          }
        }

        update_graph(root);

        for (size_t skin = 0; skin < skins.size(); ++skin) {
          auto& skin_impl = skins[skin];
          auto& anims_list = skin_anims[skin].animations.at(anim.name);
          anims_list.reserve(anims_list.size() + skin_impl.nodes.size());

          for (size_t node = 0; node < skin_impl.nodes.size(); ++node) {
            anims_list.emplace_back(skin_impl.nodes[node]->m_world_matrix * skin_impl.inv_bind_poses[node]);
          }
        }
      }
    }

    return skin_anims;
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

  void load_meshes(const tinygltf::Model& mdl, const std::shared_ptr<scene_node>& root /*, const std::vector<animation>& anim */, gl::scene::scene& s)
  {
    auto anim_tex_idx = s.textures.size() - 1;

    auto add_tex = [&s](gl::texture<GL_TEXTURE_2D>& t, gl::scene::material& m, const std::string& s_name) {
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
            const auto& img_handler = mdl.images.at(tex_handler.source);
            gl_tex.fill(reinterpret_cast<const uint8_t*>(img_handler.image.data()), img_handler.width, img_handler.height, img_handler.component);
          };

          const auto& mat_handler = mdl.materials.at(primitive.material);

          auto& m = s.materials.emplace_back(0);

          if (mat_handler.normalTexture.index >= 0) {
            gl::texture<GL_TEXTURE_2D> normal;
            load_img(mat_handler.normalTexture, normal);
            add_tex(normal, m, "s_normal");
          }

          if (mat_handler.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            gl::texture<GL_TEXTURE_2D> albedo;
            load_img(mat_handler.pbrMetallicRoughness.baseColorTexture, albedo);
            add_tex(albedo, m, "s_albedo");
          }

          if (mat_handler.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            gl::texture<GL_TEXTURE_2D> metallic_roughness;
            load_img(mat_handler.pbrMetallicRoughness.metallicRoughnessTexture, metallic_roughness);
            add_tex(metallic_roughness, m, "s_mrao");
          }

          m.add_texture("s_env", 0);
          m.add_texture("s_ibl_diff", 1);
          m.add_texture("s_ibl_spec", 2);
          m.add_texture("s_brdf", 3);
          m.add_texture("s_anim", anim_tex_idx);

          m.set_state({
              {true, true, true, true},
              true,
              gl::scene::depth_func::leq,
              gl::scene::cull_func::back,
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

gl::scene::scene gltf::load_scene(const std::string& file_name, const std::string& env_tex_name, bool is_glb)
{
  tinygltf::Model mdl;
  tinygltf::TinyGLTF loader;

  std::string err_msg;
  std::string warn_msg;

  gl::scene::scene s;

  s.camera.fov = glm::radians(90.f);
  s.camera.near = 0.1;
  s.camera.far = 10000;
  s.camera.up = {0, 1, 0};
  s.camera.forward = {0, 0, 1};
  s.camera.position = {0, 0, -3};

  auto env_tex = loader::load_tex_cube(env_tex_name);
  auto ibl_diff = loader::load_diff_ibl(env_tex);
  auto [ibl_spec, ibl_brdf] = loader::load_spec_ibl(env_tex);

  s.textures.emplace_back(std::move(env_tex));
  s.textures.emplace_back(std::move(ibl_diff));
  s.textures.emplace_back(std::move(ibl_spec));
  s.textures.emplace_back(std::move(ibl_brdf));

  s.textures.emplace_back(gl::texture<GL_TEXTURE_2D>{}); // color attachment texture
  const auto color_attachment_idx = s.textures.size() - 1;
  s.textures.emplace_back(gl::texture<GL_TEXTURE_2D>{}); // depth attachment texture
  const auto depth_attachment_idx = s.textures.size() - 1;

  s.attachments.emplace_back(s, color_attachment_idx, gl::scene::attachment_type::rgba8);
  s.attachments.emplace_back(s, depth_attachment_idx, gl::scene::attachment_type::depth24f);

  s.fbos.emplace_back();

  auto& fb = s.framebuffers.emplace_back(s, s.fbos.size() - 1, 800, 600);

  fb.add_attachment(fb.color1, 0);
  fb.add_attachment(fb.depth, 1);

  s.passes.emplace_back(s, s.framebuffers.size() - 1);

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
    true,
    gl::scene::depth_func::leq,
    gl::scene::cull_func::off,
  });

  s.vertex_sources.emplace_back(std::move(cube_vao));
  s.meshes.emplace_back(0, -1, -1, -1, 36);
  auto& n = s.nodes.emplace_back();
  n.mesh_idx = 0;
  n.material_idx = 0;

  bool load_success = false;

  if (!is_glb) {
    load_success = loader.LoadASCIIFromFile(&mdl, &err_msg, &warn_msg, file_name);
  } else {
    load_success = loader.LoadBinaryFromFile(&mdl, &err_msg, &warn_msg, file_name);
  }

  if (!load_success) {
    throw std::runtime_error(err_msg);
  }

  if (!warn_msg.empty()) {
    std::cerr << warn_msg << std::endl;
  }

  auto root = make_graph(mdl);

  auto skins = get_skins(mdl, root);
  auto anims = get_animations(mdl, root);
  auto anims_matrices = calculate_animations(anims, skins, root);

  for (size_t i = 0; i < skins.size(); ++i) {
    for (auto& data : anims_matrices[i].animations) {
      gl::texture<GL_TEXTURE_2D> anim_tex;
      const auto width = skins[i].nodes.size() * 4;
      const auto height = data.second.size() / skins[i].nodes.size();
      assert(width * height == data.second.size() * 4);
      anim_tex.fill(glm::value_ptr(data.second.front()), width, height, 4, false);
      s.textures.emplace_back(std::move(anim_tex));
    }
  }

  load_meshes(mdl, root, s);

  return s;
}
