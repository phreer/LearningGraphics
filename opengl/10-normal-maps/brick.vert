#version 330 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_tex_coord;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec3 in_bitangent;


out VS_OUT {
  vec3 frag_pos;
  vec2 tex_coord;
  vec3 tangent_light_pos;
  vec3 tangent_view_pos;
  vec3 tangent_frag_pos;
} vs_out;

uniform vec3 light_pos;
unifrom vec3 view_pos;

void main(void) {
  vec3 T = normalize(vec3(model * vec4(in_tangent, 0.0)));
  vec3 B = normalize(vec3(model * vec4(in_bitangent, 0.0)));
  vec3 N = normalize(vec3(model * vec4(in_normal, 0.0)));
  mat3 TBN = transpose(mat3(T, B, N));

  vs_out.frag_pos = vec3(u_model * vec4(in_pos, 1.0));
  vs_out.tex_coord = in_tex_coord;
  vs_out.tangent_frag_pos = TBN * vs_out.frag_pos;
  vs_out.tangent_light_pos = TBN * light_pos;
  vs_out.tangent_view_pos = TBN * view_pos;
}
