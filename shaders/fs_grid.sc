$input v_worldPos

#include "bgfx_shader.sh"

SAMPLER2D(s_gridTex,0);
uniform vec4 u_gridInfo;

#define u_gridSize u_gridInfo.xy
#define u_cellSize (1.0 / u_gridInfo.z)


void main()
{
    ivec2 uv = clamp(ivec2(v_worldPos.xz / u_cellSize), ivec2(0,0), ivec2(u_gridSize));

    float occupancy = 0.5;
    vec4 color = mix(vec4(0.0), vec4(1.0, 0.0, 0.0, 0.5), occupancy);

    gl_FragColor = color;
}
