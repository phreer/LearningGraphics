#version 330 core

in vec2 tex_cord;
in vec3 out_color;

out vec4 FragColor;

uniform sampler2D u_tex0;
uniform sampler2D u_tex1;

void main(void) {
  vec4 tex_color0 = texture(u_tex0, tex_cord);
  vec4 tex_color1 = texture(u_tex1, tex_cord);
  FragColor = mix(tex_color0, tex_color1, 0.5);
}
