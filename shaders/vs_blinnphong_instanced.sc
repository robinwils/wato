$input a_position, a_normal, a_texcoord0, i_data0, i_data1, i_data2, i_data3
$output v_worldPos, v_view, v_normal, v_texcoord0

#include "bgfx_shader.sh"

void main()
{
    // Reconstruct model matrix from instance data
    mat4 model = mtxFromCols(i_data0, i_data1, i_data2, i_data3);

    vec4 pos = vec4(a_position, 1.0);
    vec4 normal = vec4(a_normal.xyz, 0.0);

    // Transform to world space using instance model matrix
    vec3 wpos = mul(model, pos).xyz;
    v_worldPos = wpos;

    // Compute clip-space position (model already applied, use viewProj)
    gl_Position = mul(u_viewProj, vec4(wpos, 1.0));

    // Eye position in world space
    vec3 weyepos = mul(vec4(0.0, 0.0, 0.0, 1.0), u_view).xyz;
    v_view = weyepos - wpos;

    // Transform normal to world space
    vec3 wnormal = mul(model, normal).xyz;
    v_normal = normalize(wnormal);

    v_texcoord0 = a_texcoord0;
}
