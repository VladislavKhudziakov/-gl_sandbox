#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <assimp_handlers.hpp>
#include <gltf/camera.hpp>

#include <glm/gtx/euler_angles.hpp>

#include <gltf/gltf_parser.hpp>

#include <gltf/common_images_builder.hpp>
#include <gltf/common_mesh_builder.hpp>
#include <gltf/adj_mesh_builder.hpp>
#include <gltf/common_material_builder.hpp>
#include <gltf/custom_material_builder.hpp>
#include <gltf/common_parameters_builder.hpp>
#include <gltf/common_drawables_builder.hpp>
#include <gltf/common_commands_builder.hpp>



struct light_source
{
    glm::vec3 color;
    glm::vec3 position;
};

static_assert(sizeof(light_source) == sizeof(glm::vec3) * 2);

constexpr light_source light_sources[]{
    {{300.f, 300.f, 300.f}, {-10.0f, 10.0f, -10.0f}},
    {{300.f, 300.f, 300.f}, {10.0f, 10.0f, -10.0f}},
    {{300.f, 300.f, 300.f}, {-10.0f, -10.0f, -10.0f}},
    {{300.f, 300.f, 300.f}, {10.0f, -10.0f, -10.0f}},
};

glm::vec3 view_pos{0.0f, 0.0f, 2.0f};
glm::vec3 view_dir{0.0f, 0.0f, 0.0f};
glm::vec3 camera_dir{0.0f, 0.0f, -1.0f};

glm::vec3 rot{0.0f, 0.0f, 0.0f};

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
    rot = {pitch_angle, -yaw_angle, 0};
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto offset_vec = glm::normalize(camera_dir) * float(yoffset);
    view_pos += offset_vec;
}

int main()
{
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

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    {
        float anim_key = 0;
        gl::scene::scene scene;

        gltf::gltf_parser p {
            {
                std::make_unique<gltf::adj_mesh_builder>(),
                std::make_unique<gltf::custom_material_builder>(),
                std::make_unique<gltf::common_parameters_builder>(),
                std::make_unique<gltf::common_images_builder>(),
                std::make_unique<gltf::common_drawables_builder>(),
                std::make_unique<gltf::common_commands_builder>()
            }
        };

        p.parse(
            "/Users/vladislavkhudiakov/Downloads/sphere2/scene.gltf",
            "/Users/vladislavkhudiakov/Documents/dev/gl_sandbox/models/hdr/newport_loft.hdr",
            scene);

        gltf::camera cam(0, 1, scene);

        assert(glGetError() == GL_NO_ERROR);

        while (!glfwWindowShouldClose(window)) {
            {
                process_camera(window, scene);
                cam.m_position = view_pos;
                cam.m_direction = view_pos + camera_dir;
                int32_t window_fb_width, window_fb_height;
                glfwGetFramebufferSize(window, &window_fb_width, &window_fb_height);
                cam.update(window_fb_width, window_fb_height);

                auto rotation_x = glm::rotate(glm::mat4{1}, rot.x, {1.f, 0.f, 0.f});
                auto rotation_y = glm::rotate(glm::mat4{1}, rot.y, {0.f, 1.f, 0.f});
                auto rotation_z = glm::rotate(glm::mat4{1}, rot.z, {0.f, 0.f, 1.f});

                auto rotation = rotation_z * rotation_y * rotation_x;

                for (const auto& mat : scene.materials) {
                    const auto& mat_params = mat.get_parameters();
                    auto mvp_it = mat_params.find("u_MVP");

                    if (mvp_it != mat_params.end()) {
                        auto& param = scene.parameters.at(mvp_it->second);
                        auto mvp = cam.m_proj_matrix * cam.m_view_matrix * rotation;
                        std::memcpy(param.get_data(), glm::value_ptr(mvp), sizeof(mvp));
                    }

                    auto model_it = mat_params.find("u_MODEL");

                    if (model_it != mat_params.end()) {
                        auto& param = scene.parameters.at(model_it->second);
                        std::memcpy(param.get_data(), glm::value_ptr(rotation), sizeof(rotation));
                    }

                    auto anim_it = mat_params.find("u_ANIM_KEY");
                    if (anim_it != mat_params.end()) {
                        auto& param = scene.parameters.at(anim_it->second);
                        auto anim_ptr = (int32_t*) (param.get_data());
                        int32_t iaim_key = anim_key;
                        *anim_ptr = iaim_key;
                    }
                }

                gl::scene::draw(scene, window_fb_width, window_fb_height);
                glfwSwapBuffers(window);
                glfwPollEvents();
                anim_key += 0.5;
            }
        }

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    return 0;
}
