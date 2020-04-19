#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <gl_handlers.hpp>
#include <gltf_handlers.hpp>
#include <primitives.hpp>
#include <shaders.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <third/tinygltf/tiny_gltf.h>

struct light_source
{
  glm::vec3 color;
  glm::vec3 position;
};

static_assert(sizeof(light_source) == sizeof(glm::vec3) * 2);

constexpr light_source light_sources[] {
    {{200.0f, 200.0f, 200.0f}, {0.0f,  0.0f, 49.5f}},
    {{100.f, 100.0f, 100.0f}, {-1.4f, -1.9f, 9.0f}},
    {{0.0f, 0.0f, 0.2f}, {0.0f, -1.8f, 4.0f}},
    {{0.0f, 0.1f, 0.0f}, {0.8f, -1.7f, 6.0f}},
};


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
      gl::vertex_array_object cube_vao;
      {
        gl::buffer<GL_ARRAY_BUFFER> buf;
        buf.fill(primitives::cube_vertices, sizeof(primitives::cube_vertices));
        auto s = sizeof(primitives::cube_vertices);
        cube_vao.add_vertex_array<float>(buf, 3,sizeof(float[8]));
        cube_vao.add_vertex_array<float>(buf, 3,sizeof(float[8]), sizeof(float[3]));
        cube_vao.add_vertex_array<float>(buf, 2,sizeof(float[8]), sizeof(float[6]));
      }


      gl::program cube_program {
        gl::shader<GL_VERTEX_SHADER>{shaders::hdr_v_shader_source},
        gl::shader<GL_FRAGMENT_SHADER>{shaders::hdr_f_shader_source}};

      gl::vertex_array_object plane_vao;
      {
        gl::buffer<GL_ARRAY_BUFFER> buf;
        buf.fill(primitives::plane_vertices, sizeof(primitives::plane_vertices));
        cube_vao.add_vertex_array<float>(buf, 3,sizeof(float[3]));
        cube_vao.add_vertex_array<float>(buf, 2,sizeof(float[2]), sizeof(float[3]) * 4);
        cube_vao.add_vertex_array<float>(buf, 3,sizeof(float[3]), sizeof(float[3]) * 4 + sizeof(float[2]) * 4);
      }

      gl::buffer<GL_ELEMENT_ARRAY_BUFFER> plane_ebo;
      plane_ebo.fill(primitives::plane_indices, sizeof(primitives::plane_indices));

      assert(glGetError() == GL_NO_ERROR);

      auto porjection = glm::perspectiveFov(glm::radians(45.0f), 800.0f, 600.0f, 0.1f, 100.0f);
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(0.0f, 0.0f, 25.0));
      model = glm::scale(model, glm::vec3(2.5f, 2.5f, 27.5f));
      auto camera = glm::lookAt(glm::vec3{2.0f, 0.0f, 25.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

      int32_t w, h, c;
      auto wood_texture_source = stbi_load("../models/wood.png", &w, &h, &c, 0);
      gl::texture<GL_TEXTURE_2D> wood_texture;
      wood_texture.fill<uint8_t>(wood_texture_source, w, h, c == 3, false);
      stbi_image_free(wood_texture_source);

      gl::framebuffer fb(800, 600);
      fb.add_attachment<gl::framebuffer::attachment_target::color1, gl::attachment_type::color_rgba16f>();
      fb.add_attachment<gl::framebuffer::attachment_target::depth, gl::attachment_type::depth_24f>();
      gl::pass pass{std::move(fb)};

      pass.set_state({
        true,
        gl::depth_func::leq,
        true,
        {true, true, true, true},
        gl::cull_func::front,
        {0.2f, 0.3f, 0.3f, 1.0f}});

      auto mvp = porjection * camera * model;

      glEnable(GL_DEPTH_TEST);

      float anim_key = 0;

      gl::texture<GL_TEXTURE_2D> animation_texture;

      while (!glfwWindowShouldClose(window)) {
        {

          gl::bind_guard pass_guard(pass);

          cube_vao.bind();
          cube_program.bind();
          wood_texture.bind();
          cube_program.set_uniform("projection", porjection);
          cube_program.set_uniform("view", camera);
          cube_program.set_uniform("model", model);
          cube_program.set_uniform("diffuseTexture", 0);

          for (int i = 0; i < 4; ++i) {
            cube_program.set_uniform("lights[" + std::to_string(i) + "].Position", light_sources[i].position);
            cube_program.set_uniform("lights[" + std::to_string(i) + "].Color", light_sources[i].color);
          }
          glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        int32_t window_fb_width, window_fb_height;
        glfwGetFramebufferSize(window, &window_fb_width, &window_fb_height);
        pass.get_framebuffer().blit(window_fb_width, window_fb_height);

        glfwSwapBuffers(window);
        glfwPollEvents();
      }
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
