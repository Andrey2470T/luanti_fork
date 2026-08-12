#version 150

precision mediump float;

uniform int uTextureUsage;
uniform sampler2D uTextureUnit;

in vec2 vTextureCoord;
in vec4 vVertexColor;

out vec4 outColor[8];

void main()
{
	vec4 Color = vVertexColor;

	if (bool(uTextureUsage))
		Color *= texture2D(uTextureUnit, vTextureCoord);

	outColor[0] = Color;
	outColor[1] = vec4(0.0);
	outColor[2] = vec4(0.0);
	outColor[3] = vec4(0.0);
	outColor[4] = vec4(0.0);
	outColor[5] = vec4(0.0);
	outColor[6] = vec4(0.0);
	outColor[7] = vec4(0.0);
}
