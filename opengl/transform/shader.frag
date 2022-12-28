#version 330 core

in vec2 text_cord;
in vec3 out_color;

out vec4 color;

uniform sampler2D u_text;

void main(void) {
  color = vec4(out_color, 1.0);
  FragColor = texture(u_text, text_cord);
}
