#version 150

precision mediump float;

uniform int uTextureUsage;
uniform sampler2D uTextureUnit;

in vec2 vTextureCoord;
in vec4 vVertexColor;

out vec4 outColor0;

void main()
{
	vec4 Color = vVertexColor;

	if (bool(uTextureUsage))
		Color *= texture2D(uTextureUnit, vTextureCoord);

	outColor0 = Color;
}
