#ifdef SKINNED
$input a_position, a_normal, a_texcoord0, a_weight, a_indices
#elif defined(INSTANCED)
$input a_position, a_normal, a_texcoord0, i_data0, i_data1, i_data2, i_data3
#else
$input a_position, a_normal, a_texcoord0
#endif
$output v_worldPos, v_view, v_normal, v_texcoord0

#include "bgfx_shader.sh"

#ifdef SKINNED
uniform mat4 u_bones[128];
#endif

void main()
{
#ifdef INSTANCED
    // Reconstruct model matrix from instance data
    mat4 model  = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
    vec4 pos    = vec4(a_position, 1.0);
    vec4 normal = vec4(a_normal.xyz, 0.0);
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
    vec4 normal = mul(boneMat, a_normal);
#else
    mat4 model  = u_model[0];
    vec4 pos    = vec4(a_position, 1.0);
    vec4 normal = vec4(a_normal.xyz, 0.0);
#endif

    // lighting calculations are done in world space, so we send the
    // fragment position in world coords to the fragment shader
    vec3 wpos  = mul(model, pos).xyz;
    v_worldPos = wpos;

    gl_Position = mul(u_viewProj, vec4(wpos, 1.0));

    // eye position in world space
    vec3 weyepos = mul(vec4(0.0, 0.0, 0.0, 1.0), u_view).xyz;
    v_view = weyepos - wpos;
    // tangent space view dir
    // v_view = mul(weyepos - wpos, tbn);

    // transform normal to world space (rotation + uniform scale)
    v_normal    = normalize(mul(model, normal).xyz);
    v_texcoord0 = a_texcoord0;
}
