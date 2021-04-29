$input v_normal, v_frag_world_pos

uniform vec3 u_lightDir;
uniform vec3 u_cameraPos;

#include <../bgfx_shader.sh>

void main() {
    vec3 normal = normalize(v_normal);
    vec3 to_camera = normalize(u_cameraPos - v_frag_world_pos);
    float rim_amount = 1.0 - max(0.0, dot(normal, to_camera));
    rim_amount = pow(rim_amount, 4.0);
    float light_amount = max(0.0, dot(normal, normalize(u_lightDir.xyz)));
    vec3 color = abs(normal);
    gl_FragColor = vec4(color * light_amount /* + rim_amount */, 1.0);
    // gl_FragColor = vec4(normalize(abs(to_camera)), 1.0); // debug
}
