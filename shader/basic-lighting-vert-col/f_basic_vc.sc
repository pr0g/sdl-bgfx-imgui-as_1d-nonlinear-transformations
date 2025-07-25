$input v_normal, v_frag_pos, v_color0

uniform vec3 u_lightPos;
uniform vec3 u_cameraPos;

#include <../bgfx_shader.sh>

void main() {
  vec3 normal = normalize(v_normal);
  vec3 light_dir = normalize(u_lightPos - v_frag_pos);
  vec3 diffuse = max(dot(normal, light_dir), 0.0) * vec3(1.0, 1.0, 1.0);

  float ambient_strength = 0.25;
  vec3 ambient = ambient_strength * vec3(1.0, 1.0, 1.0);
  vec3 result = (ambient + diffuse) * v_color0;
  gl_FragColor = vec4(result, 1.0);
}
