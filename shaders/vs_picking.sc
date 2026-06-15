#ifdef SKINNED
$input a_position, a_weight, a_indices
#elif defined(INSTANCED)
$input a_position, i_data0, i_data1, i_data2, i_data3, i_data4
#else
$input a_position
#endif
$output v_id

#include "bgfx_shader.sh"

#ifdef SKINNED
uniform mat4 u_bones[128];
#endif

#ifndef INSTANCED
uniform vec4 u_id;
#endif

void main()
{
#ifdef INSTANCED
    // Reconstruct model matrix from instance data
    mat4 model  = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
    vec4 pos    = vec4(a_position, 1.0);
    v_id = i_data4;
#elif defined(SKINNED)
    mat4 boneMat = mtxFromCols(
        vec4_splat(0.0),
        vec4_splat(0.0),
        vec4_splat(0.0),
        vec4_splat(0.0)
    );
    if (a_weight.x > 0.0) boneMat += a_weight.x * u_bones[int(a_indices.x)];
    if (a_weight.y > 0.0) boneMat += a_weight.y * u_bones[int(a_indices.y)];
    if (a_weight.z > 0.0) boneMat += a_weight.z * u_bones[int(a_indices.z)];
    if (a_weight.w > 0.0) boneMat += a_weight.w * u_bones[int(a_indices.w)];

    mat4 model  = u_model[0];
    vec4 pos    = mul(boneMat, vec4(a_position, 1.0));
    v_id = u_id;
#else
    mat4 model  = u_model[0];
    vec4 pos    = vec4(a_position, 1.0);
    v_id = u_id;
#endif

    vec3 wpos  = mul(model, pos).xyz;
    gl_Position = mul(u_viewProj, vec4(wpos, 1.0));
}
