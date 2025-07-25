$input a_position, a_normal, a_color0
$output v_normal, v_frag_pos, v_color0

#include <../bgfx_shader.sh>

void main() {
  gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
  v_frag_pos = mul(u_model[0], vec4(a_position, 1.0)).xyz;
  v_normal = mul(u_model[0], vec4(a_normal, 0.0)).xyz;
  v_color0 = a_color0;
}
