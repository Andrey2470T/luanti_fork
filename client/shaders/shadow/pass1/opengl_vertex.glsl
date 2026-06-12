#include<shadow_vertex>

uniform mat4 LightMVP; // world matrix
out vec4 tPos;

CENTROID_ out mediump vec2 varTexCoord;

void main()
{
	vec4 pos = LightMVP * vec4(inPosition, 1.0);

	tPos = applyPerspectiveDistortion(pos);

	gl_Position = vec4(tPos.xyz, 1.0);
	varTexCoord = (mTexture * vec4(inTexCoord0.xy, 0.0, 1.0)).st;
}
