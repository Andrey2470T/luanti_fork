uniform vec2 texelSize0;

#ifdef GL_ES
out mediump vec2 varTexCoord;
#else
centroid out vec2 varTexCoord;
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
	varTexCoord.st = inTexCoord0.st;
	sampleNW = varTexCoord.st + vec2(-1.0, -1.0) * texelSize0;
	sampleNE = varTexCoord.st + vec2(1.0, -1.0) * texelSize0;
	sampleSW = varTexCoord.st + vec2(-1.0, 1.0) * texelSize0;
	sampleSE = varTexCoord.st + vec2(1.0, 1.0) * texelSize0;
	gl_Position = vec4(inPosition, 1.0);
}
