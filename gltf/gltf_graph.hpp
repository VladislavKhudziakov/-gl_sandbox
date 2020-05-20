

#pragma once

#include <memory>
#include <vector>
#include <functional>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

namespace tinygltf
{
    class Model;
}

namespace gltf
{
    class scene_graph
    {
    public:
        class node
        {
        public:
            friend gltf::scene_graph;

            glm::vec3 translation{0, 0, 0};
            glm::vec3 scale{1, 1, 1};
            glm::quat rotation{1, 0, 0, 0};

            int32_t skin_index = -1;

            int32_t mesh_index = -1;

            glm::mat4 get_local_transformation() const;
            glm::mat4 get_global_transformation() const;
            uint32_t get_node_index() const;

        private:
            glm::mat4 m_world_matrix{1.};
            std::vector<std::shared_ptr<node>> m_children;
            std::weak_ptr<node> m_parent;
            uint32_t m_node_index{0};
        };

        scene_graph(const tinygltf::Model& mdl, uint32_t scene_index);
        ~scene_graph();
        void update();
        std::shared_ptr<node> get_root() const;

        void go_though(std::function<bool(const std::shared_ptr<node>&)>) const;
        void go_though(std::function<bool(std::shared_ptr<node>&)>);

    private:
        void make_scene_graph();
        void make_scene_node(uint32_t node, std::shared_ptr<scene_graph::node>& parent);

        void for_each_node(
            std::shared_ptr<scene_graph::node>& graph_node,
            const std::function<bool(std::shared_ptr<scene_graph::node>&)>& f);

        void for_each_node(
            const std::shared_ptr<scene_graph::node>& graph_node, const std::function<bool(const std::shared_ptr<scene_graph::node>&)>& f) const;

        uint32_t m_scene_index;
        std::shared_ptr<node> m_root;
        const tinygltf::Model& m_model;
    };
} // namespace gltf
