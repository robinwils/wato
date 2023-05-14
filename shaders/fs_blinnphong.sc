$input v_worldPos, v_view, v_normal, v_texcoord0

#include "common/common.sh"

SAMPLER2D(s_diffuseTex,  0);

uniform vec4 u_lightDir;
uniform vec4 u_lightCol;

void main()
{
	vec3 lightDir = normalize(-u_lightDir);  
	float diff = max(dot(v_normal, lightDir), 0.0);

	vec3 diffuse = diff * u_lightCol.xyz * texture2D(s_diffuseTex, v_texcoord0).rgb;

	// specular

	gl_FragColor.xyz = diffuse;
	gl_FragColor.w = 1.0;
	gl_FragColor = toGamma(gl_FragColor);
	//gl_FragColor = texture2D(s_diffuseTex, v_texcoord0);
	//gl_FragColor.xyz = dot(v_normal, lightDir) * u_lightCol.xyz;
}
