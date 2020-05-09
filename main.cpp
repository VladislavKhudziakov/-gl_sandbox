#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <gltf_handlers.hpp>
#include <assimp_handlers.hpp>

#include <glm/gtx/euler_angles.hpp>

struct light_source
{
  glm::vec3 color;
  glm::vec3 position;
};

static_assert(sizeof(light_source) == sizeof(glm::vec3) * 2);

constexpr light_source light_sources[] {
    {{300.f,   300.f,  300.f}, {-10.0f,  10.0f, -10.0f}},
    {{300.f,   300.f,  300.f}, {10.0f,  10.0f, -10.0f}},
    {{300.f,   300.f,  300.f}, {-10.0f, -10.0f, -10.0f}},
    {{300.f,   300.f,  300.f}, {10.0f, -10.0f, -10.0f}},
};

glm::vec3 view_pos {0.0f, 0.0f, 12.0f};
glm::vec3 view_dir {0.0f, 0.0f, 0.0f};
glm::vec3 camera_dir {0.0f, 0.0f, -1.0f};

constexpr uint32_t rows_count = 7;
constexpr uint32_t columns_count = 7;
constexpr float spacing = 2.5;

void process_camera(GLFWwindow* window, gl::scene::scene& s)
{
  static bool is_first = true;

  static double yaw_angle = 0.0;
  static double pitch_angle = 0.0;

  static double last_x = 0;
  static double last_y = 0;

  double x, y;
  int32_t w, h;

  glfwGetWindowSize(window, &w, &h);
  glfwGetCursorPos(window, &x, &y);

  if (is_first) {
    last_x = w / 2.;
    last_y = h / 2.;
    is_first = false;
    return;
  }

  auto x_offset = last_x - x;
  auto y_offset = last_y - y;

  last_x = x;
  last_y = y;

  int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

  if (state != GLFW_PRESS) {
    return;
  }

  yaw_angle += (x_offset / w) * 10;
  pitch_angle -= (y_offset / h) * 10;

  s.nodes[1].transformation.rotation = {pitch_angle, 0, -yaw_angle};
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto offset_vec = glm::normalize(camera_dir) * float(yoffset);
  view_pos += offset_vec;
}

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

  glfwSetScrollCallback(window, scroll_callback);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    {
      auto scene = gltf::load_scene("/Users/vladislavkhudiakov/Downloads/scifi_drone_1.1/scene.gltf", "/Users/vladislavkhudiakov/Documents/dev/gl_sandbox/models/hdr/newport_loft.hdr");

      while (!glfwWindowShouldClose(window)) {
        {
          process_camera(window, scene);
          scene.camera.position = view_pos;
          scene.camera.forward = view_pos + camera_dir;
          gl::scene::draw(scene, {0, 1}, 0);
          int32_t window_fb_width, window_fb_height;
          glfwGetFramebufferSize(window, &window_fb_width, &window_fb_height);
          scene.framebuffers.at(scene.passes.front().get_framebuffer_idx()).blit(window_fb_width, window_fb_height);
          glfwSwapBuffers(window);
          glfwPollEvents();
        }
      }

      glfwDestroyWindow(window);
      glfwTerminate();
    }

    return 0;
}
