layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;
layout (location = 4) in int materialType;

#include <matrices>
#include <final_light_color>

#ifdef ENABLE_DYNAMIC_SHADOWS
#include <shadows>
#endif

uniform float mDayNightRatio;
uniform float mAnimationTimer;

out vec3 vNormal;
out vec3 vPosition;
out vec3 vWorldPosition;
out lowp vec4 vColor;
#ifdef GL_ES
out mediump ivec2 vTexCoord;
#else
centroid out ivec2 vTexCoord;
#endif

#ifdef ENABLE_DYNAMIC_SHADOWS
	out float vCosLight;
	out float vAdjShadowStrength;
	out float vNormalLength;
	out vec3 vShadowPosition;
	out float vPerspectiveFactor;
#endif

out highp vec3 vEyeVec;
out float vNightRatio;
out float vIDiff;
const float e = 2.718281828459;
const float BS = 10.0;

#ifdef ENABLE_DYNAMIC_SHADOWS

vec4 getRelativePosition(in vec4 position)
{
	vec2 l = position.xy - ShadowParams.cameraPos.xy;
	vec2 s = l / abs(l);
	s = (1.0 - s * ShadowParams.cameraPos.xy);
	l /= s;
	return vec4(l, s);
}

float getPerspectiveFactor(in vec4 relativePosition)
{
	float pDistance = length(relativePosition.xy);
	float pFactor = pDistance * ShadowParams.xyPerspectiveBias0 + ShadowParams.xyPerspectiveBias1;
	return pFactor;
}

vec4 applyPerspectiveDistortion(in vec4 position)
{
	vec4 l = getRelativePosition(position);
	float pFactor = getPerspectiveFactor(l);
	l.xy /= pFactor;
	position.xy = l.xy * l.zw + ShadowParams.cameraPos.xy;
	position.z *= ShadowParams.zPerspectiveBias;
	return position;
}

// custom smoothstep implementation because it's not defined in glsl1.2
// https://docs.gl/sl4/smoothstep
float mtsmoothstep(in float edge0, in float edge1, in float x)
{
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}
#endif


float directional_ambient(vec3 normal)
{
	vec3 v = normal * normal;

	if (normal.y < 0.0)
		return dot(v, vec3(0.670820, 0.447213, 0.836660));

	return dot(v, vec3(0.670820, 1.000000, 0.836660));
}

void main(void)
{
	vTexCoord = uv;//(Matrices.texture0 * vec4(texCoords.xy, 1.0, 1.0)).st;

	gl_Position = Matrices.worldViewProj * vec4(pos, 1.0);

	vPosition = gl_Position.xyz;
	vNormal = (Matrices.world * vec4(skinnedNormal, 0.0)).xyz;
	vWorldPosition = (Matrices.world * vec4(pos, 1.0)).xyz;
	vEyeVec = -(Matrices.worldView * vec4(pos, 1.0)).xyz;

	if (materialType == TILE_MATERIAL_PLAIN) || (materialType == TILE_MATERIAL_PLAIN_ALPHA)
		vIDiff = 1.0;
	else {
		// This is intentional comparison with zero without any margin.
		// If normal is not equal to zero exactly, then we assume it's a valid, just not normalized vector
		vIDiff = length(skinnedNormal) == 0.0
			? 1.0
			: directional_ambient(normalize(skinnedNormal));
	}

	// Calculate color.
	float nightRatio = 1.0 - color.a;
	vColor = finalLightColor(mDayNightRatio, color, nightRatio);
	vNightRatio = nightRatio;

#ifdef ENABLE_DYNAMIC_SHADOWS
	if (ShadowParams.shadow_strength > 0.0) {
		vec3 nNormal = normalize(vNormal);
		vNormalLength = length(vNormal);

		/* normalOffsetScale is in world coordinates (1/10th of a meter)
		   z_bias is in light space coordinates */
		float normalOffsetScale, z_bias;
		float pFactor = getPerspectiveFactor(getRelativePosition(ShadowParams.shadowViewProj * pos));
		if (vNormalLength > 0.0) {
			nNormal = normalize(vNormal);
			vCosLight = max(1e-5, dot(nNormal, -ShadowParams.lightDirection));
			float sinLight = pow(1.0 - pow(vCosLight, 2.0), 0.5);
			normalOffsetScale = 0.1 * pFactor * pFactor * sinLight * min(ShadowParams.shadowfar, 500.0) /
					ShadowParams.xyPerspectiveBias1 / ShadowParams.textureresolution;
			z_bias = 1e3 * sinLight / vCosLight * (0.5 + ShadowParams.textureresolution / 1024.0);
		}
		else {
			nNormal = vec3(0.0);
			vCosLight = clamp(dot(ShadowParams.lightDirection, normalize(vec3(ShadowParams.lightDirection.x, 0.0, ShadowParams.lightDirection.z))), 1e-2, 1.0);
			float sinLight = pow(1.0 - pow(vCosLight, 2.0), 0.5);
			normalOffsetScale = 0.0;
			z_bias = 3.6e3 * sinLight / vCosLight;
		}
		z_bias *= pFactor * pFactor / ShadowParams.textureresolution / ShadowParams.shadowfar;

		// Possible breakage: the skinned position is in the absolute coords, not in the relative
		vShadowPosition = applyPerspectiveDistortion(ShadowParams.shadowViewProj * vec4(pos + normalOffsetScale * nNormal, 1.0)).xyz;
		//vShadowPosition = applyPerspectiveDistortion(ShadowParams.shadowViewProj * Matrices.world * (vec4(pos, 1.0) + vec4(normalOffsetScale * nNormal, 0.0))).xyz;
		vShadowPosition.z -= z_bias;
		vPerspectiveFactor = pFactor;

		if (ShadowParams.timeofday < 0.2) {
			vAdjShadowStrength = ShadowParams.shadow_strength * 0.5 *
				(1.0 - mtsmoothstep(0.18, 0.2, ShadowParams.timeofday));
		} else if (ShadowParams.timeofday >= 0.8) {
			vAdjShadowStrength = ShadowParams.shadow_strength * 0.5 *
				mtsmoothstep(0.8, 0.83, ShadowParams.timeofday);
		} else {
			vAdjShadowStrength = ShadowParams.shadow_strength *
				mtsmoothstep(0.20, 0.25, ShadowParams.timeofday) *
				(1.0 - mtsmoothstep(0.7, 0.8, ShadowParams.timeofday));
		}
	}
#endif
}
