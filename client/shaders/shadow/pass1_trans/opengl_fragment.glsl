#include<shadow_common>

uniform sampler2D ColorMapSampler;
in vec4 tPos;

#ifdef COLORED_SHADOWS
in vec3 varColor;
#endif

CENTROID_ in mediump vec2 varTexCoord;

void main()
{
	vec4 col = texture2D(ColorMapSampler, varTexCoord);
#ifndef COLORED_SHADOWS
	if (col.a < 0.5)
		discard;
#endif

	float depth = 0.5 + tPos.z * 0.5;
	// ToDo: Liso: Apply movement on waving plants
	// depth in [0, 1] for texture

	//col.rgb = col.a == 1.0 ? vec3(1.0) : col.rgb;
#ifdef COLORED_SHADOWS
	col.rgb *= varColor.rgb;
	// premultiply color alpha (see-through side)
	float packedColor = packColor(col.rgb * (1.0 - col.a));
	outputColor(vec4(depth, packedColor, 0.0, 1.0));
#else
	outputColor(vec4(depth, 0.0, 0.0, 1.0));
#endif
}
