$input v_worldPos

#include "bgfx_shader.sh"

SAMPLER2D(s_gridTex,0);
uniform vec3 u_gridInfo;

#define u_gridSize u_diffuse.xy
#define u_cellSize 1.0 / u_diffuse.z


void main()
{
    vec2 local = v_worldPos.xz;
    ivec2 uv = int(local / u_cellSize);

    float occupancy = texelFetch(s_gridTex, uv, 1).r;
    vec4 color = mix(vec4(0.0), vec4(1.0, 0.0, 0.0, 0.5), occupancy);

    gl_FragColor = texture2D(s_gridTex, uv);
}
