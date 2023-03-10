#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_tex_cord;
layout (location = 2) in vec3 a_norm;

out vec2 tex_cord;
out vec3 out_color;
out vec3 out_norm;
// For calculating lighting
out vec3 out_frag_pos;

uniform vec3 u_color;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main(void) {
  gl_Position = u_projection * u_view * u_model * vec4(a_pos, 1.0);
  out_color = u_color;
  tex_cord = a_tex_cord;
  out_norm = mat3(transpose(inverse(u_model))) * a_norm;
  out_frag_pos = vec3(u_model * vec4(a_pos, 1.0));
}
