#version 100

/* Attributes */

attribute vec2 inVertexPosition;
attribute vec4 inVertexColor;
attribute vec2 inTexCoord0;

/* Uniforms */

uniform float uThickness;
uniform mat4 uProjection;

/* Varyings */

varying vec2 vTextureCoord;
varying vec4 vVertexColor;

void main()
{
	gl_Position = uProjection * vec4(inVertexPosition, 1.0, 1.0);
	gl_PointSize = uThickness;
	vTextureCoord = inTexCoord0;
	vVertexColor = inVertexColor.bgra;
}
