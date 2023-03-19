#version 330 core

layout (location = 0) in vec3 a_pos;

out vec3 tex_coords;

uniform mat4 u_projection;
uniform mat4 u_view;

void main() {
  tex_coords = a_pos;
  gl_Position = (u_projection * u_view * vec4(a_pos, 1.0)).xyww;
}
