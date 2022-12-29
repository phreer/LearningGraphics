#version 330 core

in vec2 tex_cord;
in vec3 out_color;
in vec3 out_norm;
in vec3 out_frag_pos;

out vec4 FragColor;

uniform sampler2D u_tex0;
uniform sampler2D u_tex1;
uniform vec3 u_light_pos;
uniform vec3 u_light_color;
uniform vec3 u_camera_pos;

void main(void) {
  vec4 tex_color0 = texture(u_tex0, tex_cord);
  vec4 tex_color1 = texture(u_tex1, tex_cord);
  vec3 tex_color = vec3(mix(tex_color0, tex_color1, 0.5));

  vec3 norm = normalize(out_norm);
  vec3 light_dir = normalize(u_light_pos - out_frag_pos);
  float diff = max(dot(norm, light_dir), 0.0);
  vec3 diffuse = diff * u_light_color;

  float ambient_strength = 0.3;
  vec3 ambient = ambient_strength * u_light_color;
  
  float specular_strength = 5;
  vec3 camera_dir = normalize(u_camera_pos - out_frag_pos);
  vec3 reflect_dir = reflect(-light_dir, out_norm);
  float spec = pow(max(dot(camera_dir, reflect_dir), 0.0), 32);
  vec3 specular = specular_strength * spec * u_light_color;

  FragColor = vec4((ambient + diffuse + specular) * tex_color, 1.0);
}
