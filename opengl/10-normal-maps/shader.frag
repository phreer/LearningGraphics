#version 330 core
struct Light {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct Material {
  sampler2D texture_diffuse1;
  sampler2D texture_specular1;
};

uniform Material material;
uniform Light light;
uniform vec3 u_camera_pos;

in vec2 tex_cord;
in vec3 out_norm;
in vec3 out_frag_pos;

out vec4 FragColor;

void main(void) {
  vec4 diffuse1 = texture(material.texture_diffuse1, tex_cord);
  vec4 specular1 = texture(material.texture_specular1, tex_cord);

  float diffuse_strength = 1;
  vec3 norm = normalize(out_norm);
  vec3 light_dir = normalize(light.position - out_frag_pos);
  float diff = max(dot(norm, light_dir), 0.0);
  vec3 diffuse = diffuse_strength * diff * light.diffuse * diffuse1.rgb;

  float ambient_strength = 0.2;
  vec3 ambient = ambient_strength * light.ambient;
  
  float specular_strength = 0.8;
  vec3 camera_dir = normalize(u_camera_pos - out_frag_pos);
  vec3 reflect_dir = reflect(-light_dir, out_norm);
  float spec = pow(max(dot(camera_dir, reflect_dir), 0.0), 4);
  vec3 specular = specular_strength * spec * light.specular * specular1.rgb;

  FragColor = vec4(ambient + diffuse + specular, 1.0);
}
