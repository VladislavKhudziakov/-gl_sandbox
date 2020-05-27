

#pragma once

namespace shaders
{
    inline constexpr static auto* hdr_plane_vss = R"(#version 410 core
    layout (location=0) in vec3 a_pos;
    layout (location=1) in vec2 a_uv;

    out vec2 var_uv;

    void main()
    {
      gl_Position = vec4(a_pos, 1.);
      var_uv = a_uv;
    }
  )";

    inline constexpr static auto* blur_fss = R"(#version 410 core
    layout (location=0) out vec4 frag_color;

    in vec2 var_uv;
    uniform sampler2D s_blur_tex;
    uniform vec2 u_blur_axis;
    float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

    void main()
    {
      vec3 color = texture(s_blur_tex, var_uv).rgb * weight[0];
      vec2 pixel_size = 1. / vec2(textureSize(s_blur_tex, 0));

      for (int i = 1; i < 5; ++i) {
        color += texture(s_blur_tex, vec2(var_uv) + vec2(pixel_size) * float(i) * u_blur_axis).rgb * weight[i];
        color += texture(s_blur_tex, vec2(var_uv) + vec2(pixel_size) * float(i) * -u_blur_axis).rgb * weight[i];
      }

      frag_color = vec4(color, 1.);
    }
  )";

    inline constexpr static auto* hdr_plane_fss = R"(#version 410 core
    layout (location=0) out vec4 frag_color;
    in vec2 var_uv;

    uniform sampler2D s_process_texture;
    uniform sampler2D s_bloom_texture;
    uniform float u_time;

    float exposure = 1.;

    void main()
    {
      vec4 org_color = texture(s_process_texture, var_uv);
//      vec4 bloom_color = texture(s_bloom_texture, var_uv);
//      org_color += bloom_color;
      org_color.rgb = org_color.rgb / (org_color.rgb + vec3(1.));
      org_color.rgb = vec3(1.0) - exp(-org_color.rgb * exposure);
      org_color.rgb = pow(org_color.rgb, vec3(1. / 2.2));
      frag_color = vec4(org_color.rgb, 1.);
    }
  )";

    inline constexpr static auto* deferred_lighting_fss = R"(#version 410 core
    layout (location=0) out vec4 frag_color;
    in vec2 var_uv;

    uniform sampler2D s_positions;
    uniform sampler2D s_normals;
    uniform sampler2D s_albedo_spec;
    uniform sampler2D s_occl;


    uniform float u_time;

    float exposure = 1.;

    struct light {
      vec3 position;
      vec3 color;
    };

    uniform light u_light_sources[16];
    uniform vec3 u_view_pos;

    void main() {
      vec4 albedo_spec = texture(s_albedo_spec, var_uv);
      vec3 vpos = texture(s_positions, var_uv).xyz;
      vec3 N = texture(s_normals, var_uv).xyz;
      vec3 occl = texture(s_occl, var_uv).xyz;

      vec3 ambient = albedo_spec.rgb * 0.3 * occl;

      for (int i = 0; i < 16; ++i) {
        vec3 light_dir = normalize(u_light_sources[i].position - vpos);
        float diff_coeff = max(dot(light_dir, N), 0);
        vec3 diff_component = diff_coeff * u_light_sources[i].color * albedo_spec.rgb;

        vec3 res = diff_component;

        float distance = length(vpos - u_light_sources[i].position);
        float attenuation = 1. / pow(distance, 2.);
        res *= attenuation;

        vec3 view_dir = normalize(u_view_pos - vpos);
        vec3 reflect_dir = reflect(-light_dir, N);
        float spec_coeff = pow(max(dot(view_dir, reflect_dir), 0.), 32);
        res += spec_coeff * attenuation * u_light_sources[i].color * albedo_spec.a;

        ambient += res;
      }

      vec4 org_color = vec4(ambient, 1.);
//      vec4 org_color = texture(s_process_texture, var_uv);
//      vec4 bloom_color = texture(s_bloom_texture, var_uv);
//      org_color += bloom_color;
//      org_color.rgb = org_color.rgb / (org_color.rgb + vec3(1.));
      org_color.rgb = vec3(1.0) - exp(-org_color.rgb * exposure);
      org_color.rgb = pow(org_color.rgb, vec3(1. / 2.2));
      frag_color = vec4(org_color.rgb, 1.);
      frag_color = vec4(occl, 1.);
    }
  )";

    inline constexpr static auto* ssao_fss = R"(#version 410 core
    layout (location=0) out vec4 frag_color;
    in vec2 var_uv;

    uniform sampler2D s_positions;
    uniform sampler2D s_normals;
    uniform sampler2D s_noise;
    uniform sampler2D s_samples;

    uniform vec2 u_screen_size;
    uniform mat4 u_PROJ;
    uniform float u_time;

    float radius = 1;

    void main() {
      vec2 screen_scale = vec2(800, 600) / 4.;

      vec3 vpos = texture(s_positions, var_uv).xyz;
      vec3 N = normalize(texture(s_normals, var_uv).xyz);
      vec3 T = normalize(texture(s_noise, var_uv * screen_scale).xyz);
      T = normalize(T - N * dot(T, N));
      vec3 B = cross(T, N);
      mat3 TBN = mat3(T, B, N);

      float occl = 0.;

      for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
          vec3 sample_pos = texelFetch(s_samples, ivec2(i, j), 0).xyz;
          sample_pos = TBN * sample_pos;
          sample_pos = vpos + sample_pos * radius;

          vec4 sample_normalized = u_PROJ * vec4(sample_pos, 1.);
          sample_normalized.xyz /= sample_normalized.w;
          sample_normalized.xyz = sample_normalized.xyz * 0.5 + 0.5;
          float scene_depth = texture(s_positions, sample_normalized.xy).z;
          float sample_depth = sample_pos.z;
          float range_check = smoothstep(0.0, 1.0, radius / abs(vpos.z - scene_depth));
          occl += (scene_depth >= sample_depth + 0.025 ? 1. : 0.) * range_check;
        }
      }
      occl = max(1. - (occl / 64.), 0.);
      frag_color = vec4(occl, occl, occl, 1.);
    }
  )";


    inline constexpr static auto* geometry_bloom_vss = R"(#version 410 core
    layout (location = 0) in vec3 a_pos;
    layout (location = 1) in vec3 a_n;
    layout (location = 2) in vec2 a_uv;

    uniform mat4 u_MVP;
    uniform mat4 u_M;

    out vec2 var_uv;
    out vec3 var_n;
    out vec3 var_vpos;

    void main()
    {
      gl_Position = u_MVP * vec4(a_pos, 1.);
      var_uv = a_uv;
      var_n = vec3(normalize(u_M * vec4(a_n, 0.)));
      var_vpos = vec3(u_M * vec4(a_pos, 1.));
    }
  )";

    inline constexpr static auto* geometry_bloom_fss = R"(#version 410 core
    layout (location=0) out vec4 orig_color;
    layout (location=1) out vec4 bloom_map;

    in vec2 var_uv;
    in vec3 var_n;
    in vec3 var_vpos;

    struct Light {
      vec3 Position;
      vec3 Color;
    };

    uniform sampler2D s_diffuse;
    uniform sampler2D s_specular;
    uniform Light[16] u_light_sources;

    void main()
    {
      vec3 diffuse_color = texture(s_diffuse, var_uv).rgb;
      vec3 spec_coeff = texture(s_specular, var_uv).rgb;
      vec3 lighting = vec3(0.);

      for(int i = 0; i < 16; i++) {
        vec3 light_dir = normalize(u_light_sources[i].Position - var_vpos);
        float diff = max(dot(light_dir, var_n), 0.0);
        vec3 diffuse = u_light_sources[i].Color * diff * diffuse_color;
        vec3 result = diffuse;

        float distance = length(var_vpos - u_light_sources[i].Position);
        result *= (1.0 / (distance * distance));
        lighting += result;
      }

      orig_color = vec4(lighting, 1.);
      float brightness = dot(lighting, vec3(0.2126, 0.7152, 0.0722));
      bloom_map = vec4(lighting * step(1., brightness), 1.);
    }
  )";

    inline constexpr static auto* normal_map_v_shader_source = R"(#version 410 core
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

    inline constexpr static auto* normal_map_f_shader_source = R"(#version 410 core
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

    inline constexpr static auto* hdr_v_shader_source = R"(#version 410 core
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

    inline constexpr static auto* hdr_f_shader_source = R"(#version 410 core
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

    inline constexpr static auto* cyborg_vss = R"(#version 410 core
layout (location = 0) in vec3 attr_pos;
layout (location = 1) in vec3 attr_normal;
layout (location = 2) in vec3 attr_tangent;
layout (location = 3) in vec2 attr_uv;

out vec2 var_uv;
out mat3 var_tbn;
out vec3 var_vpos;


uniform mat4 u_MVP;
uniform mat4 u_M;

void main()
{
  gl_Position = u_MVP * vec4(attr_pos, 1.);
  vec3 N = normalize(vec3(u_M * vec4(attr_normal, 0.)));
  vec3 T = normalize(vec3(u_M * vec4(attr_tangent, 0.)));
  vec3 B = cross(N, T);
  var_tbn = mat3(T, B, N);

  var_vpos = vec3(u_M * vec4(attr_pos, 1.));
  var_uv = attr_uv;
}
)";

    inline constexpr static auto* cyborg_fss = R"(#version 410 core
layout (location = 0) out vec4 frag_color;

in vec2 var_uv;
in mat3 var_tbn;
in vec3 var_vpos;

uniform sampler2D s_albedo;
uniform sampler2D s_normal;
uniform sampler2D s_specular;

struct light {
    vec3 position;
    vec3 color;
};

uniform light u_light_sources[16];
uniform vec3 u_view_pos;

void main()
{
  vec4 base_color = texture(s_albedo, var_uv);
  vec3 N = normalize(var_tbn * (texture(s_normal, var_uv).xyz * 2. - 1.));
  float spec_str = texture(s_specular, var_uv).x;

  vec3 ambient = base_color.rgb * 0.1;

  for (int i = 0; i < 16; ++i) {
    vec3 light_dir = normalize(u_light_sources[i].position - var_vpos);
    float diff_coeff = max(dot(N, light_dir), 0.);
    vec3 diff_color = diff_coeff * u_light_sources[i].color * base_color.rgb;

    float dist = length(var_vpos - u_light_sources[i].position);
    float attenuation = 1. / pow(dist, 2);

    diff_color *= attenuation;

    vec3 view_dir = normalize(u_view_pos - var_vpos);
    vec3 reflect_dir = normalize(reflect(-light_dir, N));
    vec3 spec_coeff = pow(max(dot(view_dir, reflect_dir), 0), 22) * spec_str * u_light_sources[i].color;

    diff_color += spec_coeff * attenuation;

    ambient += diff_color;
  }

  frag_color = vec4(ambient, base_color.a);
}
)";

    inline constexpr static auto* cyborg_deferred_vss = R"(#version 410 core
layout (location = 0) in vec3 attr_pos;
layout (location = 1) in vec3 attr_normal;
layout (location = 2) in vec3 attr_tangent;
layout (location = 3) in vec2 attr_uv;

out vec2 var_uv;
out mat3 var_tbn;
out vec3 var_vpos;


uniform mat4 u_MVP;
uniform mat4 u_M;

uniform float u_invert_normals;

void main()
{
  gl_Position = u_MVP * vec4(attr_pos, 1.);
  vec3 N = normalize(vec3(u_M * vec4(attr_normal * u_invert_normals, 0.)));
  vec3 T = normalize(vec3(u_M * vec4(attr_tangent, 0.)));
  vec3 B = cross(N, T);
  var_tbn = mat3(T, B, N);

  var_vpos = vec3(u_M * vec4(attr_pos, 1.));
  var_uv = attr_uv;
}
)";

    inline constexpr static auto* cyborg_deferred_fss = R"(#version 410 core
layout (location = 0) out vec4 vert_pos;
layout (location = 1) out vec4 normal;
layout (location = 2) out vec4 albedo_specular;

in vec2 var_uv;
in mat3 var_tbn;
in vec3 var_vpos;

uniform sampler2D s_albedo;
uniform sampler2D s_normal;
uniform sampler2D s_specular;

void main()
{
  vec4 base_color = texture(s_albedo, var_uv);
  vec3 N = normalize(var_tbn * (texture(s_normal, var_uv).xyz * 2. - 1.));
  float spec_str = texture(s_specular, var_uv).x;

  vert_pos = vec4(var_vpos, 1.);
  normal = vec4(N, 1.);
  albedo_specular = vec4(base_color.rgb, spec_str);
}
)";


    inline constexpr static auto* pbr_vss = R"(#version 410 core
layout (location = 0) in vec3 attr_pos;
layout (location = 1) in vec2 attr_uv;
layout (location = 2) in vec3 attr_normal;
layout (location = 3) in vec3 attr_tangent;
layout (location = 4) in vec4 attr_bones;
layout (location = 5) in vec4 attr_weights;

//#define ANIM

out vec2 var_uv;
out vec3 var_v;
out vec3 var_n;
out vec3 var_t;
out vec3 var_b;

uniform mat4 u_MVP;
uniform mat4 u_MODEL;

#ifdef ANIM
uniform int u_ANIM_KEY;
uniform sampler2D s_anim;

mat4 get_anim_matrix(int bone_idx, int key)
{
  vec4 x = texelFetch(s_anim, ivec2(bone_idx, key), 0);
  vec4 y = texelFetch(s_anim, ivec2(bone_idx + 1, key), 0);
  vec4 z = texelFetch(s_anim, ivec2(bone_idx + 2, key), 0);
  vec4 w = texelFetch(s_anim, ivec2(bone_idx + 3, key), 0);
  return mat4(x, y, z, w);
}

mat4 get_anim_transform(int key)
{
  mat4 m1 = get_anim_matrix(int(attr_bones.x) * 4, key) * attr_weights.x;
  mat4 m2 = get_anim_matrix(int(attr_bones.y) * 4, key) * attr_weights.y;
  mat4 m3 = get_anim_matrix(int(attr_bones.z) * 4, key) * attr_weights.z;
  mat4 m4 = get_anim_matrix(int(attr_bones.w) * 4, key) * attr_weights.w;

  return m1 + m2 + m3 + m4;
}

#endif

void main()
{
#ifdef ANIM
  mat4 anim_transform = get_anim_transform(u_ANIM_KEY);
  mat4 model_transform = u_MODEL * anim_transform;
  vec3 pos = vec3(anim_transform * vec4(attr_pos, 1.));
#else
  vec3 pos = attr_pos;
  mat4 model_transform = u_MODEL;
#endif

  gl_Position = u_MVP * vec4(pos, 1.);
  var_n = vec3(model_transform * vec4(attr_normal, 0.));

  var_t = vec3(model_transform * vec4(attr_tangent, 0.));
  var_t = normalize(var_t - var_n * max(dot(var_n, var_t), 0.));
  var_b = cross(var_n, var_t);

  var_v = vec3(model_transform * vec4(attr_pos, 1.));
  var_uv = attr_uv;
}
)";

    inline constexpr static auto* pbr_fss = R"(#version 410 core
layout (location = 0) out vec4 frag_color;

in vec2 var_uv;
in vec3 var_v;
in vec3 var_n;
in vec3 var_t;
in vec3 var_b;

struct light
{
  vec3 position;
  vec3 color;
};

uniform float u_roughness;
uniform float u_metalness;
uniform vec3 u_albedo;
uniform vec3 u_cam_pos;

uniform sampler2D s_albedo;
uniform sampler2D s_mrao;
uniform sampler2D s_normal;

uniform light u_light_sources[4];

const float PI = 3.14159265359;

vec3 fresnel_schlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);

    return NdotV;
}

vec3 g2l(vec3 src)
{
  return pow(vec3(1.) - exp(-src * 1.), vec3(1. / 2.2));
}

void main()
{
  vec3 N = normalize(mat3(var_t, var_b, var_n) * (texture(s_normal, var_uv).xyz * 2. - 1.));

  vec3 V = normalize(u_cam_pos - var_v);
  vec3 F0 = vec3(0.04);
  vec3 albedo = texture(s_albedo, var_uv).rgb;
  vec4 mrao = texture(s_mrao, var_uv);

  float metallic = mrao.r;
  float roughness = mrao.g;

  F0 = mix(F0, albedo, metallic);

  vec3 Lo = vec3(0.0);
  for (int i = 0; i < 4; ++i) {
    vec3 L = normalize(u_light_sources[i].position - var_v);
    vec3 H = normalize(V + L);
    float dist = length(u_light_sources[i].position - var_v);
    vec3 R = u_light_sources[i].color * (1. / pow(dist, 2));

    vec3 F = fresnel_schlick(clamp(dot(H, V), 0.0, 1.0), F0);
    float D = distribution_ggx(N, H, roughness);
    float G = geometry_smith(N, V, L, roughness);

    vec3 nom = F * D * G;
    float denom = 4. * max(max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.001);
    vec3 BRDF = nom / denom;
    vec3 kS = F;
    vec3 kD = (1. - kS) * (1. - metallic);

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + BRDF) * R * NdotL;
  }

  vec3 ambeint = albedo * 0.03 + Lo;

  frag_color = vec4(g2l(ambeint), 1.);
}
)";

    inline constexpr static auto* solid_fss = R"(#version 410 core
layout (location = 0) out vec4 frag_color;

in vec2 var_uv;

vec3 g2l(vec3 src)
{
  return pow(vec3(1.) - exp(-src * 1.), vec3(1. / 2.2));
}

uniform sampler2D u_color;
void main()
{
  frag_color = texture(u_color, var_uv);
}
)";

    inline constexpr static auto* cubemap_bg_fss = R"(#version 410 core
layout (location = 0) out vec4 frag_color;

in vec3 v_pos;

uniform samplerCube s_bg;
void main()
{
  vec3 bg_color = texture(s_bg, v_pos).rgb;
  bg_color = sqrt(vec3(1.) - exp(-bg_color));
  frag_color = vec4(bg_color, 1.);
}
)";

    inline constexpr static auto* cubemap_bg_vss = R"(#version 410 core
layout (location = 0) in vec3 attr_pos;

out vec3 v_pos;

uniform mat4 u_PROJECTION;
uniform mat4 u_VIEW;

void main()
{
  mat4 view = mat4(mat3(u_VIEW));
  vec4 clip_pos = u_PROJECTION * view * vec4(attr_pos, 1.);
  gl_Position = clip_pos.xyww;
  v_pos = attr_pos;
}
)";

    inline constexpr static auto* pbr_fss_ibl = R"(#version 410 core
layout (location = 0) out vec4 frag_color;

in vec2 var_uv;
in vec3 var_v;
in vec3 var_n;
in vec3 var_t;
in vec3 var_b;

struct light
{
  vec3 position;
  vec3 color;
};

uniform float u_roughness;
uniform float u_metalness;
uniform vec3 u_albedo;
uniform vec3 u_cam_pos;

uniform sampler2D s_albedo;
uniform sampler2D s_metallic_roughness;
uniform sampler2D s_normal;
uniform samplerCube s_ibl_diff;
uniform samplerCube s_ibl_spec;
uniform sampler2D s_brdf;

uniform light u_light_sources[4];

const float PI = 3.14159265359;

vec3 fresnel_schlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

vec3 fresnel_schlick_roughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);

    return NdotV;
}

vec3 g2l(vec3 src)
{
  return pow(vec3(1.) - exp(-src * 1.), vec3(1. / 2.2));
}

void main()
{
  vec3 N = normalize(mat3(var_t, var_b, var_n) * (texture(s_normal, var_uv).xyz * 2. - 1.));
  vec3 V = normalize(u_cam_pos - var_v);
  vec3 R = reflect(-V, N);
  vec3 F0 = vec3(0.04);
  vec3 albedo = texture(s_albedo, var_uv).rgb;
  vec4 mrao = texture(s_metallic_roughness, var_uv);

  float metallic = mrao.r;
  float roughness = mrao.g;

  F0 = mix(F0, albedo, metallic);

  vec3 F = fresnel_schlick_roughness(max(dot(N, V), 0.0), F0, roughness);
  vec3 kS = F;
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallic;
  vec3 irradiance = texture(s_ibl_diff, N).rgb;
  vec3 diffuse = irradiance * albedo;

  const float MAX_REFLECTION_LOD = 4.0;
  vec3 prefiltered_color = textureLod(s_ibl_spec, R, roughness * MAX_REFLECTION_LOD).rgb;
  vec2 brdf  = texture(s_brdf, vec2(max(dot(N, V), 0.0), roughness)).rg;
  vec3 specular = prefiltered_color * (F * brdf.x + brdf.y);

  vec3 final_color = kD * diffuse + specular;

  frag_color = vec4(g2l(final_color), 1.);
}
)";
} // namespace shaders
