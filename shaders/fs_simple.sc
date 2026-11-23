$input v_color0, v_texcoord0

/*
 * Copyright 2011-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "bgfx_shader.sh"

SAMPLER2D(s_tex,0);


void main()
{
	gl_FragColor = v_color0 * texture2D(s_tex, v_texcoord0);
}
