#version 320 core

layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 color;

uniform float mThickness;
uniform mat4 mProjection;

out vec4 vVertexColor;

void main()
{
	gl_Position = mProjection * vec4(pos, 1.0, 1.0);
	gl_PointSize = mThickness;
	vVertexColor = color.bgra;
}
