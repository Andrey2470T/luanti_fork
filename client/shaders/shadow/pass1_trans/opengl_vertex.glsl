#include<shadow_vertex>

uniform mat4 LightMVP; // world matrix
out vec4 tPos;
#ifdef COLORED_SHADOWS
out vec3 varColor;
#endif

CENTROID_ out mediump vec2 varTexCoord;

void main()
{
	vec4 pos = LightMVP * vec4(inPosition, 1.0);

	tPos = applyPerspectiveDistortion(pos);

	gl_Position = vec4(tPos.xyz, 1.0);
	varTexCoord = inTexCoord0.st;

#ifdef COLORED_SHADOWS
	varColor = inColor.rgb;
#endif
}
