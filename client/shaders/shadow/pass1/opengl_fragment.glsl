#include<base_texture>

in vec4 tPos;

CENTROID_ in mediump vec2 varTexCoord;

void main()
{
#ifdef USE_ATLAS
	vec4 col = getTextureColor(varTexCoord, 1, 0, vec2(0.0));
#else
	vec4 col = getTextureColor(varTexCoord, 0, 0, vec2(0.0));
#endif

	if (col.a < 0.70)
		discard;

	float depth = 0.5 + tPos.z * 0.5;
	outputColor(vec4(depth, 0.0, 0.0, 1.0));
}
