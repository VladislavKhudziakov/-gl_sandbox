

#include "gltf_parser.hpp"

#include <gltf/misc/acessor_utils.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <third/tinygltf/tiny_gltf.h>

namespace
{
    struct anim_keys
    {
        std::vector<glm::vec3> translations;
        std::vector<glm::vec3> scales;
        std::vector<glm::quat> rotations;
    };

    struct anim
    {
        std::string name;
        std::vector<anim_keys> keys;
        std::vector<std::shared_ptr<gltf::scene_graph::node>> nodes;
        uint32_t keys_size;
    };

    void get_anims(std::vector<anim>& keys, const tinygltf::Model& m, const gltf::scene_graph& graph)
    {
        for (const auto& anim : m.animations) {
            auto curr_anim_keys = keys.emplace_back();
            for (const auto& channel : anim.channels) {
                const auto& sampler = anim.samplers.at(channel.sampler);
                auto node_it = std::find_if(
                    curr_anim_keys.nodes.begin(), curr_anim_keys.nodes.end(), [&](const std::shared_ptr<gltf::scene_graph::node>& node) {
                        return channel.target_node == node->get_node_index();
                    });

                uint32_t idx;

                if (node_it == curr_anim_keys.nodes.end()) {
                    curr_anim_keys.keys.emplace_back();

                    auto& curr_node = curr_anim_keys.nodes.emplace_back();

                    graph.go_though([&curr_node, &channel](const std::shared_ptr<gltf::scene_graph::node>& n) {
                        if (n->get_node_index() == channel.target_node) {
                            curr_node = n;
                            return false;
                        }

                        return true;
                    });

                    idx = curr_anim_keys.nodes.size() - 1;
                } else {
                    idx = node_it - curr_anim_keys.nodes.begin();
                }

                if (channel.target_path == "translation") {
                    gltf::utils::copy_buffer_data([](glm::vec3& data) { return glm::value_ptr(data); }, curr_anim_keys.keys.at(idx).translations, m, sampler.output);
                } else if (channel.target_path == "scale") {
                    gltf::utils::copy_buffer_data([](glm::vec3& data) { return glm::value_ptr(data); }, curr_anim_keys.keys.at(idx).scales, m, sampler.output);
                } else if (channel.target_path == "rotation") {
                    gltf::utils::copy_buffer_data([](glm::quat& data) { return glm::value_ptr(data); }, curr_anim_keys.keys.at(idx).rotations, m, sampler.output);
                }
            }

            for (const auto& key : curr_anim_keys.keys) {
                auto curr_size = key.translations.size();
                curr_size = std::max(curr_size, key.scales.size());
                curr_size = std::max(curr_size, key.rotations.size());
                curr_anim_keys.keys_size = std::max(uint32_t(curr_size), curr_anim_keys.keys_size);
            }
        }
    }
} // namespace


gltf::gltf_parser::gltf_parser(const tinygltf::Model& mdl)
    : m_model(mdl)
    , m_graph(mdl, 0)
{
}


gltf::scene_graph& gltf::gltf_parser::get_graph()
{
    return m_graph;
}


const gltf::scene_graph& gltf::gltf_parser::get_graph() const
{
    return m_graph;
}


void gltf::gltf_parser::parse()
{
    m_graph.go_though([this](std::shared_ptr<gltf::scene_graph::node>& n) {
        const auto& mdl_node = m_model.nodes.at(n->get_node_index());

        int32_t skin_index = -1;

        if (mdl_node.skin >= 0) {
            m_skins.emplace_back(m_model, m_model.skins.at(mdl_node.skin), m_graph);
            skin_index = m_skins.size() - 1;
            n->skin_index = skin_index;
        }

        if (mdl_node.mesh >= 0) {
            m_meshes.emplace_back(m_model, m_model.meshes.at(mdl_node.mesh), skin_index);
            n->mesh_index = m_meshes.size() - 1;
        }

        return true;
    });

    calculate_animations();
}


const std::vector<gltf::skin>& gltf::gltf_parser::get_skins() const
{
    return m_skins;
}


const std::vector<gltf::mesh>& gltf::gltf_parser::get_meshes() const
{
    return m_meshes;
}


void gltf::gltf_parser::calculate_animations()
{
    std::vector<anim> anims;

    get_anims(anims, m_model, m_graph);

    std::sort(anims.begin(), anims.end(), [](const anim& r, const anim& l) {
        return r.nodes.size() < l.nodes.size();
    });

    auto make_skins_anims = [this](const std::string& name) {
        for (int skin = 0; skin < m_skins.size(); ++skin) {
            auto& skin_impl = m_skins[skin];

            auto& anim = skin_impl.animations.emplace_back();
            anim.name = name;
            anim.keys.reserve(skin_impl.get_nodes().size());

            const auto& nodes = skin_impl.get_nodes();
            const auto& inv_bind_poses = skin_impl.get_nodes_matrices();

            for (int i = 0; i < nodes.size(); ++i) {
                anim.keys.emplace_back(nodes[i]->get_global_transformation() * inv_bind_poses[i]);
            }
        }
    };

    make_skins_anims("hierarcy");


    for (const auto& anim : anims) {
        for (size_t key = 0; key < anim.keys_size; ++key) {
            for (size_t node = 0; node < anim.nodes.size(); ++node) {
                auto& node_impl = anim.nodes[node];
                const auto& node_key = anim.keys[node];

                if (key < node_key.translations.size()) {
                    node_impl->translation = node_key.translations[key];
                }

                if (key < node_key.scales.size()) {
                    node_impl->scale = node_key.scales[key];
                }

                if (key < node_key.rotations.size()) {
                    node_impl->rotation = node_key.rotations[key];
                }
            }

            m_graph.update();

            make_skins_anims(anim.name);
        }
    }
}
