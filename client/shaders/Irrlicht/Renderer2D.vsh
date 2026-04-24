#version 150

in vec2 inPosition;
in vec4 inColor;
in vec2 inTexCoord0;

uniform float uThickness;
uniform mat4 uProjection;

out vec2 vTextureCoord;
out vec4 vVertexColor;

void main()
{
	gl_Position = uProjection * vec4(inPosition, 1.0, 1.0);
	gl_PointSize = uThickness;
	vTextureCoord = inTexCoord0;
	vVertexColor = inColor.bgra;
}
