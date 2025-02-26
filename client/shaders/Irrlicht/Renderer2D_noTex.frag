#version 320 core

precision mediump float;

/* Input */

in vec4 outColor;

/* Output */

out vec4 color;

void main()
{
	color = outColor;
}
