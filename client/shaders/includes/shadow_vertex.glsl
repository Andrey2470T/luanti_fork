#include<shadow_common>
#include<shadow_uniforms>

vec4 getRelativePosition(in vec4 position)
{
	vec2 l = position.xy - CameraPos.xy;
	vec2 s = l / abs(l);
	s = (1.0 - s * CameraPos.xy);
	l /= s;
	return vec4(l, s);
}

float getPerspectiveFactor(in vec4 relativePosition)
{
	float pDistance = length(relativePosition.xy);
	float pFactor = pDistance * xyPerspectiveBias0 + xyPerspectiveBias1;
	return pFactor;
}

vec4 applyPerspectiveDistortion(in vec4 position)
{
	vec4 l = getRelativePosition(position);
	float pFactor = getPerspectiveFactor(l);
	l.xy /= pFactor;
	position.xy = l.xy * l.zw + CameraPos.xy;
	position.z *= zPerspectiveBias;
	return position;
}

void vertexStage(
	in vec4 shadowPos,
	in vec3 normal,
	in vec3 lightDir,
	in float timeOfDay,
	out float normalLength,
	out float cosLight,
	out vec3 shadowPerspPos,
	out float perspFactor,
	out float adjShadowStrength
)
{
	if (f_shadow_strength <= 0.0)
		return;

	vec3 nNormal;
	normalLength = length(normal);

	/* normalOffsetScale is in world coordinates (1/10th of a meter)
		z_bias is in light space coordinates */
	float normalOffsetScale, z_bias;
	float pFactor = getPerspectiveFactor(getRelativePosition(m_ShadowViewProj * mWorld * shadowPos));
	if (normalLength > 0.0) {
		nNormal = normalize(normal);
		cosLight = max(1e-5, dot(nNormal, -lightDir));
		float sinLight = pow(1.0 - pow(cosLight, 2.0), 0.5);
		normalOffsetScale = 2.0 * pFactor * pFactor * sinLight * min(f_shadowfar, 500.0) /
			xyPerspectiveBias1 / f_textureresolution;
		z_bias = 1.0 * sinLight / cosLight;
	}
	else {
		nNormal = vec3(0.0);
		cosLight = clamp(dot(lightDir, normalize(vec3(lightDir.x, 0.0, lightDir.z))), 1e-2, 1.0);
		float sinLight = pow(1.0 - pow(cosLight, 2.0), 0.5);
		normalOffsetScale = 0.0;
		z_bias = 3.6e3 * sinLight / cosLight;
	}
	z_bias *= pFactor * pFactor / f_textureresolution / f_shadowfar;

	shadowPerspPos = applyPerspectiveDistortion(m_ShadowViewProj * mWorld * (shadowPos + vec4(normalOffsetScale * nNormal, 0.0))).xyz;
#if !defined(ENABLE_TRANSLUCENT_FOLIAGE) || MATERIAL_TYPE != TILE_MATERIAL_WAVING_LEAVES
	shadowPerspPos.z -= z_bias;
#endif
	perspFactor = pFactor;

	if (timeOfDay < 0.2) {
		adjShadowStrength = f_shadow_strength * 0.5 *
			(1.0 - mtsmoothstep(0.18, 0.2, timeOfDay));
	} else if (timeOfDay >= 0.8) {
		adjShadowStrength = f_shadow_strength * 0.5 *
			mtsmoothstep(0.8, 0.83, timeOfDay);
	} else {
		adjShadowStrength = f_shadow_strength *
			mtsmoothstep(0.20, 0.25, timeOfDay) *
			(1.0 - mtsmoothstep(0.7, 0.8, timeOfDay));
	}
}
