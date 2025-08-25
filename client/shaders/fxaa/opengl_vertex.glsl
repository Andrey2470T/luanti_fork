layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 uv;

uniform vec2 mTexelSize;

#ifdef GL_ES
out mediump vec2 vUV;
#else
centroid out vec2 vUV;
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
	sampleNW = varTexCoord.st + vec2(-1.0, -1.0) * mTexelSize;
	sampleNE = varTexCoord.st + vec2(1.0, -1.0) * mTexelSize;
	sampleSW = varTexCoord.st + vec2(-1.0, 1.0) * mTexelSize;
	sampleSE = varTexCoord.st + vec2(1.0, 1.0) * mTexelSize;
	gl_Position = vec4(pos, 0.0, 1.0);
}
