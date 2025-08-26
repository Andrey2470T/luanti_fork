layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 uv;

#define rendered mTexture0
uniform sampler2D rendered;

#ifdef GL_ES
out mediump vec2 vUV;
out mediump vec2 vTexelSize;
#else
centroid out vec2 vUV;
centroid out vec2 vTexelSize;
#endif

out vec2 sampleNW;
out vec2 sampleNE;
out vec2 sampleSW;
out vec2 sampleSE;

/*
Based on
https://github.com/mattdesl/glsl-fxaa/
Portions Copyright (c) 2011 by Armin Ronacher.
*/
void main(void)
{
	vUV = uv;
	vTexelSize = 1.0f / textureSize(rendered, 0);
	sampleNW = vUV + vec2(-1.0, -1.0) * vTexelSize;
	sampleNE = vUV + vec2(1.0, -1.0) * vTexelSize;
	sampleSW = vUV + vec2(-1.0, 1.0) * vTexelSize;
	sampleSE = vUV + vec2(1.0, 1.0) * vTexelSize;
	gl_Position = vec4(pos, 0.0, 1.0);
}
