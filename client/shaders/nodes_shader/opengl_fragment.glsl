#include<fog>
#include<noise>
#include<base_texture>

uniform vec3 lightDir;
// The cameraOffset is the current center of the visible world.
uniform highp vec3 cameraOffset;
uniform vec3 cameraPosition;
uniform float animationTimer;
#ifdef ENABLE_DYNAMIC_SHADOWS
	in float adj_shadow_strength;
	in float cosLight;
	in float f_normal_length;
	in vec3 shadow_position;
	in float perspective_factor;
#endif

in vec3 vNormal;
in vec3 vPosition;
// World position in the visible world (i.e. relative to the cameraOffset.)
// This can be used for many shader effects without loss of precision.
// If the absolute position is required it can be calculated with
// cameraOffset + worldPosition (for large coordinates the limits of float
// precision must be considered).
in vec3 worldPosition;

CENTROID_ in lowp float emissionLight;
CENTROID_ in lowp vec3 varColor;
CENTROID_ in lowp vec3 dayLight;
CENTROID_ in mediump vec2 varTexCoord;
CENTROID_ in float nightRatio;

in highp vec3 eyeVec;
in vec3 hwColor;
in vec2 crackTexCoord;
in float hasCrack;

void main(void)
{
	vec2 uv = varTexCoord.st;
	vec4 base = getTextureColor(uv, 1, hasCrack, crackTexCoord);

	DISCARD_CHECK(base);

	vec4 col = vec4(base.rgb * hwColor * varColor.rgb, 1.0);

	float f_adj_shadow_strength = 0.0;
	float shadow_strength = 0.0;
	float shadow_uncorrected = 0.0;
#ifdef ENABLE_DYNAMIC_SHADOWS
	shadow_strength = f_shadow_strength;
	col = fragmentStage(
		shadow_position, col, dayLight,
		f_normal_length, cosLight, perspective_factor,
		adj_shadow_strength, nightRatio,
		f_adj_shadow_strength, shadow_uncorrected
	);
#endif

	col = mixColorWithFog(col, eyeVec);
	col = vec4(col.rgb, base.a);

#ifndef ENABLE_BLOOM
	outputColor(col);
#else
	outputColor(col, vec4(col.rgb * emissionLight, 1.0));
#endif
}
