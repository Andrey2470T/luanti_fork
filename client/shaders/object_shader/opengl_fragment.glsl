#include<fog>

#ifdef ENABLE_DYNAMIC_SHADOWS
	in float adj_shadow_strength;
	in float cosLight;
	in float f_normal_length;
	in vec3 shadow_position;
	in float perspective_factor;
#endif


in vec3 vNormal;
in lowp vec3 varColor;
in lowp vec3 dayLight;

CENTROID_ in mediump vec2 varTexCoord;

in highp vec3 eyeVec;
in float nightRatio;

in float vIDiff;

void main(void)
{
	vec3 color;
	vec2 uv = varTexCoord.st;

	vec4 base = texture2D(baseTexture, uv).rgba;

	DISCARD_CHECK(base)

	color = base.rgb;
	vec4 col = vec4(color.rgb * varColor.rgb, 1.0);
	col.rgb *= vIDiff;

	float f_adj_shadow_strength = 0.0;
	float shadow_uncorrected = 0.0;
#ifdef ENABLE_DYNAMIC_SHADOWS
	col = fragmentStage(
		shadow_position, col, dayLight,
		f_normal_length, cosLight, perspective_factor,
		adj_shadow_strength, nightRatio,
		f_adj_shadow_strength, shadow_uncorrected
	);
#endif

	col = mixColorWithFog(col, eyeVec);
	col = vec4(col.rgb, base.a);

	output(col);
}
