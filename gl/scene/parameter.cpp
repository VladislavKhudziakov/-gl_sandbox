

#include "parameter.hpp"


gl::scene::parameter::parameter(
    gl::scene::parameter_type param_type,
    gl::scene::parameter_component_type component_type)
    : m_data(get_param_type_size(param_type) * get_param_components_size(component_type))
    , m_type(param_type)
    , m_component_type(component_type)
{
}


gl::scene::parameter::parameter(
    gl::scene::parameter_type param_type,
    gl::scene::parameter_component_type component_type,
    void* data)
    : parameter(param_type, component_type)
{
    std::memcpy(m_data.data(), data, get_param_type_size(param_type) * get_param_components_size(component_type));
}


uint8_t* gl::scene::parameter::get_data()
{
    return m_data.data();
}


const uint8_t* gl::scene::parameter::get_data() const
{
    return m_data.data();
}


gl::scene::parameter_type gl::scene::parameter::get_param_type() const
{
    return m_type;
}


gl::scene::parameter_component_type gl::scene::parameter::get_component_type() const
{
    return m_component_type;
}
