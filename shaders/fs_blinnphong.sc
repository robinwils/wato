$input v_worldPos, v_view, v_normal, v_texcoord0

#include "common/common.sh"

// w is a bool indicating to use texture or not
uniform vec4 u_diffuse;
SAMPLER2D(s_diffuseTex,0);

// w is a bool indicating to use texture or not
uniform vec4 u_specular;
SAMPLER2D(s_specularTex,1);

uniform vec4 u_lightDir;
uniform vec4 u_lightCol;

#define u_useDiffuseTex u_diffuse.w
#define u_useSpecularTex u_specular.w

void main()
{
  vec3 lightDir = normalize(-u_lightDir);
  float diff = max(dot(v_normal, lightDir), 0.0);

  vec3 diffuse = 0.0;
  if (u_useDiffuseTex) {
    diffuse = diff * u_lightCol.xyz * texture2D(s_diffuseTex, v_texcoord0).rgb;
  } else {
    diffuse = diff * u_lightCol.xyz * u_diffuse.xyz;
  }

  vec3 viewDir    = normalize(v_view - v_worldPos);
  vec3 halfwayDir = normalize(lightDir + viewDir);

  // specular
  float spec = pow(max(dot(v_normal, halfwayDir), 0.0), 16.0);
  vec3 specular = 0.0;
  if (u_useSpecularTex) {
    specular = spec * u_lightCol.xyz * texture2D(s_specularTex, v_texcoord0).rgb;
  } else {
    specular = spec * u_lightCol.xyz * u_specular.xyz;
  }

  gl_FragColor.xyz = diffuse + specular;
  gl_FragColor.w = 1.0;
  gl_FragColor = toGamma(gl_FragColor);
}
