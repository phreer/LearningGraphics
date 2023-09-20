#version 330 core

struct Light {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

out VS_OUT {
  vec3 frag_pos;
  vec2 tex_coord;
  vec3 tangent_light_pos;
  vec3 tangent_view_pos;
  vec3 tangent_frag_pos;
} fs_in;

struct Material {
  sampler2D texture_diffuse1;
  sampler2D texture_specular1;
  sampler2D texture_normal1;
};

uniform Material material;
uniform Light light;

void main(void) {
  vec4 diffuse1 = texture(material.texture_diffuse1, tex_cord);
  vec4 specular1 = texture(material.texture_specular1, tex_cord);
  vec4 normal1 = texture(material.texture_normal1, tex_cord);

  vec3 norm = normalize(normal1);
  vec3 light_dir = normalize(fs_in.tangent_light_pos - fs_in.tangent_frag_pos);
  float diff = max(dot(norm, light_dir), 0.0);
  vec3 diffuse = diffuse_strength * light.diffuse * diffuse1.rgb;
  vec3 diffuse = diffuse_strength * diff * light.diffuse * diffuse1.rgb;

  float ambient_strength = 0.2;
  vec3 ambient = ambient_strength * light.ambient;
  
  float specular_strength = 0.8;
  vec3 camera_dir = normalize(fs_in.view_pos - fs_in.frag_pos);
  vec3 reflect_dir = reflect(-light_dir, norm);
  float spec = pow(max(dot(camera_dir, reflect_dir), 0.0), 4);
  vec3 specular = specular_strength * spec * light.specular * specular1.rgb;

  FragColor = vec4(ambient + diffuse + specular, 1.0);
}
