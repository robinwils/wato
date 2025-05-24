#ifdef SKINNED
$input a_position, a_normal, a_texcoord0, a_weight, a_indices
#else
$input a_position, a_normal, a_texcoord0
#endif
$output v_worldPos, v_view, v_normal, v_texcoord0

#include "common/common.sh"

void main()
{
	// lighting calculations are done in world space, so we send the
	// fragment position in world coords to the fragment shader
	vec3 wpos = mul(u_model[0], vec4(a_position, 1.0) ).xyz;
	v_worldPos = wpos;

	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );

	// eye position in world space
	vec3 weyepos = mul(vec4(0.0, 0.0, 0.0, 1.0), u_view).xyz;
	v_view = weyepos - wpos;
	// tangent space view dir
	// v_view = mul(weyepos - wpos, tbn);


	v_normal = normalize(a_normal.xyz);
	v_texcoord0 = a_texcoord0;
}
