#version 320 core

uniform mWorld;

layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 uv;

out vec4 varColor;
out vec2 varTexCoord;

void main(void)
{
	varTexCoord = uv;
	gl_Position = mWorld * inVertexPosition;
	varColor = color;
}
