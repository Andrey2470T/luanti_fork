#version 320 core

precision mediump float;

in vec4 vVertexColor;

out vec4 outColor;

void main()
{
	outColor = vVertexColor;
}
