#version 330 core

in vec2 text_cord;
in vec3 out_color;

out vec4 FragColor;

uniform sampler2D u_text;

void main(void) {
  vec4 tex_color = texture(u_text, text_cord);
  FragColor = mix(vec4(out_color, 1.0), tex_color, 0.8);
}
