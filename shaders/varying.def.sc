vec4 v_color0    : COLOR0    = vec4(1.0, 0.0, 0.0, 1.0);
vec3 v_normal    : NORMAL    = vec3(0.0, 0.0, 1.0);
vec2 v_texcoord0 : TEXCOORD0 = vec2(0.0, 0.0);
vec3 v_worldPos  : TEXCOORD1 = vec3(0.0, 0.0, 0.0);
vec3 v_view      : TEXCOORD2 = vec3(0.0, 0.0, 0.0);

vec3  a_position  : POSITION;
vec4  a_color0    : COLOR0;
vec4  a_normal    : NORMAL;
vec2  a_texcoord0 : TEXCOORD0;
vec4  a_weight    : BLENDWEIGHT;
vec4 a_indices    : BLENDINDICES;
