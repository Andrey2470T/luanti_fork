#version 150

in vec3 inPosition;
in vec3 inNormal;
in vec4 inColor;
in vec2 inTexCoord0;

uniform mat4 uWVPMatrix;
uniform mat4 uWVMatrix;
uniform mat4 uTMatrix0;

uniform float uThickness;

out vec2 vTextureCoord0;
out vec4 vVertexColor;
out float vFogCoord;

void main()
{
	gl_Position = uWVPMatrix * vec4(inPosition, 1.0);
	gl_PointSize = uThickness;

	vec4 TextureCoord0 = vec4(inTexCoord0.x, inTexCoord0.y, 1.0, 1.0);
	vTextureCoord0 = vec4(uTMatrix0 * TextureCoord0).xy;

	vVertexColor = inColor.bgra;

	vec3 Position = (uWVMatrix * vec4(inPosition, 1.0)).xyz;

	vFogCoord = length(Position);
}
