#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <gl_handlers.hpp>
#include <gltf_handlers.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <third/tinygltf/tiny_gltf.h>

constexpr auto model_path = "../models/Dragon_2.5_For_Animations.glb";

const char* v_shader_source =
"#version 410 core\n"
"layout (location=0) in vec3 a_pos;"
"layout (location=1) in vec3 a_normal;"
"layout (location=2) in vec4 a_joints;"
"layout (location=3) in vec4 a_weights;"
"uniform mat4 u_proj;"
"uniform mat4 u_view;"
"uniform mat4 u_world;"
"uniform mat4 u_bones[166];"
"out vec3 v_normal;"
"void main() {"
"v_normal = a_normal;"
"gl_Position = u_proj * u_view *"
"(a_weights.x * u_bones[int(a_joints.x)] + "
"a_weights.y * u_bones[int(a_joints.y)] + "
"a_weights.z * u_bones[int(a_joints.z)] + "
"a_weights.w * u_bones[int(a_joints.w)]) *"
"vec4(a_pos, 1.0);"
"}";

const char* f_shader_source =
"#version 410 core\n"
"precision highp float;"
"layout (location=0) out vec4 frag_color;"
"in vec3 v_normal;"
"uniform vec3 u_color;"
"void main() {"
"vec3 light_pos = normalize(vec3(-1, 3, 5));"
"float l = dot(light_pos, normalize(v_normal)) * 0.5 + 0.5;"
"frag_color = vec4(l * u_color, 1.0);"
"}";


int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    auto window = glfwCreateWindow(800, 600, "gl sandbox", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    {
      tinygltf::Model mdl{};
      std::string err;
      std::string warn;

      tinygltf::TinyGLTF loader{};
      auto success = loader.LoadBinaryFromFile(&mdl, &err, &warn, model_path);

      std::vector<gltf::mesh> meshes;
      auto h = gltf::make_graph(mdl, meshes);
      auto skins = gltf::get_skins(mdl, h);
      auto anims = gltf::get_animations(mdl, h);

      auto perspective = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 1.0f, 2000.0f);

      gl::program program {
          gl::shader<GL_VERTEX_SHADER>(v_shader_source),
          gl::shader<GL_FRAGMENT_SHADER>(f_shader_source)
      };

      glViewport(0, 0, 800, 600);

      glEnable(GL_DEPTH_TEST);

      auto anim_key = 0;

      while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        {
          auto time = glfwGetTime();
          glm::vec3 camera_position {cos(time * .1) * 100, 30, sin(time * .1) * 100};
          const auto target = glm::vec3{0, 0, -10};
          const auto up = glm::vec3{0, 1, 0};

          auto view = glm::lookAt(camera_position, target, up);

          gl::bind_guard program_g(program);

          for (const auto& mesh : meshes) {
            gl::bind_guard vao_g(mesh.vao);
            gl::bind_guard ind_g(mesh.indices);
            program.set_uniform("u_color", glm::vec3{0, 1, 1});
            program.set_uniform("u_proj", perspective);
            program.set_uniform("u_view", view);
            program.set_uniform("u_world", mesh.world_transform);

            for (size_t j = 0; j < anims[0].nodes.size(); ++j) {
              if (!anims[0].anim_keys[j].translation.empty()) {
                anims[0].nodes[j]->set_translation(anims[0].anim_keys[j].translation[anim_key]);
              }

              if (!anims[0].anim_keys[j].scale.empty()) {
                anims[0].nodes[j]->set_scale(anims[0].anim_keys[j].scale[anim_key]);
              }

              if (!anims[0].anim_keys[j].rotation.empty()) {
                anims[0].nodes[j]->set_rotation(anims[0].anim_keys[j].rotation[anim_key]);
              }

              anim_key %= anims[0].anim_keys[j].translation.size();
            }

//            h->update_graph();

            std::vector<glm::mat4> bones;
            bones.reserve(skins[mesh.skin_idx].nodes.size());

            for (size_t i = 0; i < skins[mesh.skin_idx].nodes.size(); ++i) {
              bones.push_back(skins[mesh.skin_idx].nodes[i]->get_world_matrix() * skins[mesh.skin_idx].inv_bind_poses[i]);
            }

            program.set_uniform("u_bones", bones.data(), bones.size());

            glDrawElements(GL_TRIANGLES, mesh.indices_count, GL_UNSIGNED_SHORT, nullptr);

            anim_key++;
          }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
      }
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
