#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_color;
layout (location = 2) in vec2 a_tex_cord;

out vec2 tex_cord;
out vec3 out_color;

uniform mat4 transform;

void main(void) {
  gl_Position = transform * vec4(a_pos, 1.0);
  out_color = a_color;
  tex_cord = a_tex_cord;
}
