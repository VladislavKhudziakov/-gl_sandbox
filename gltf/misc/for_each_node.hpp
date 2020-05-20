
#pragma once

#include <functional>
#include <cinttypes>

namespace tinygltf
{
    class Model;
    class Node;
}

namespace gltf::utils
{
    void for_each_node(const tinygltf::Model& mdl, uint32_t, const std::function<void(const tinygltf::Node)>& );
}



