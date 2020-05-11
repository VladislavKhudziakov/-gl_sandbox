

#pragma once
#include <memory>
#include <vector>
#include <unordered_map>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <gl_handlers.hpp>

#include <gl/scene/scene.hpp>

namespace gltf
{
  gl::scene::scene load_scene(const std::string&, const std::string&, bool is_glb = false);
}

