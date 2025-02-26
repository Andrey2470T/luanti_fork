#version 320 core

precision mediump float;

/* Uniforms */

uniform int textureUsed;
uniform sampler2D texture;

/* Inputs */

in vec2 outUV;
in vec4 outColor;

/* Output */

out vec4 color;

void main()
{
	vec4 Color = outColor;

	if (bool(textureUsed))
		Color *= texture2D(texture, outUV);

	color = Color;
}
