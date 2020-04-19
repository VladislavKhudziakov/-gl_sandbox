

#pragma once

namespace shaders
{
  const char* normal_map_v_shader_source =R"(#version 410 core
  layout (location=0) in vec3 a_pos;
  layout (location=1) in vec2 a_uv;
  layout (location=2) in vec3 a_n;
  layout (location=3) in vec3 a_t;

  out vec2 var_uv;
  out vec3 var_norm;
  out vec3 var_vpos;
  out vec3 var_light_pos;
  out mat3 var_tbn;

  uniform mat4 u_MVP;
  uniform mat4 u_M;
  uniform float u_time;

  vec3 light_pos = vec3(0.f, 0.f, .3f);

  void main()
  {
    var_uv = a_uv;
    gl_Position = u_MVP * vec4(a_pos.xyz, 1);

    var_norm = vec3(u_M * vec4(a_n, 0.));

    vec3 N = normalize(vec3(u_M * vec4(a_n,    0.0)));
    vec3 T = normalize(vec3(u_M * vec4(a_t,   0.0)));

    T = normalize(T - dot(T, N) * N);

    vec3 B = cross(N, T);

    var_tbn = mat3(T, B, N);

  //  mat3 TBN = transpose(mat3(T, B, N));

  //  var_vpos = TBN * vec3(u_M * vec4(a_pos, 0.));
  //  var_light_pos = TBN * light_pos;
  //  var_light_pos.z += 1.;
  //  var_light_pos.y += sin(u_time) * 2.;
      light_pos.z += sin(u_time);
      var_vpos = vec3(u_M * vec4(a_pos, 1.));
      var_light_pos = light_pos;
  }
  )";

  const char* normal_map_f_shader_source = R"(#version 410 core
  layout (location=0) out vec4 frag_color;

  in vec2 var_uv;
  in vec3 var_norm;
  in vec3 var_vpos;
  in vec3 var_light_pos;
  in mat3 var_tbn;

  uniform float u_time;
  uniform sampler2D s_normal;

  void main()
  {
    vec2 uv = var_uv;
    vec3 diff_color = vec3(1., 0., 1.);
    vec3 normal = normalize(texture(s_normal, var_uv).xyz * 2. - 1.);
    normal = var_tbn * normal;
    vec3 light_dir = normalize(var_light_pos - var_vpos);
    float diff = max(dot(light_dir, normal), 0);
    diff_color *= diff;
    frag_color = vec4(diff_color, 1.);
  }
  )";

const char* hdr_v_shader_source =R"(#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform bool inverse_normals;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;

//    vec3 n = inverse_normals ? -aNormal : aNormal;
    vec3 n = -aNormal;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vs_out.Normal = normalize(normalMatrix * n);

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* hdr_f_shader_source =R"(#version 410 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

struct Light {
    vec3 Position;
    vec3 Color;
};

uniform Light lights[16];
uniform sampler2D diffuseTexture;
uniform vec3 viewPos;

void main()
{
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    // ambient
    vec3 ambient = 0.0 * color;
    // lighting
    vec3 lighting = vec3(0.0);
    for(int i = 0; i < 16; i++)
    {
        // diffuse
        vec3 lightDir = normalize(lights[i].Position - fs_in.FragPos);
        float diff = max(dot(lightDir, normal), 0.0);
        vec3 diffuse = lights[i].Color * diff * color;
        vec3 result = diffuse;
        // attenuation (use quadratic as we have gamma correction)
        float distance = length(fs_in.FragPos - lights[i].Position);
        result *= 1.0 / (distance * distance);
        lighting += result;
    }
    FragColor = vec4(ambient + lighting, 1.0);
})";
}
