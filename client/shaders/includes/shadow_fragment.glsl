#include<shadow_common>
#include<shadow_uniforms>
#include<shadow_depth>
#include<shadow_filter>

vec4 fragmentStage(
	in vec3 pos, in vec4 color, in vec3 dayLight,
	in float normalLength, in float cosLight, in float perspFactor,
	in float adjShadowStrength, in float nightRatio,
	out float adjShadowStrengthCorrected, out float shadowUncorrected
)
{
	if (f_shadow_strength <= 0.0) {
		adjShadowStrengthCorrected = 1.0;
		shadowUncorrected = 0.0;
		return color;
	}

	float shadow_int = 0.0;
	vec3 shadow_color = vec3(0.0, 0.0, 0.0);
	vec3 posLightSpace = getLightSpacePosition(pos);

	float distance_rate = (1.0 - pow(clamp(2.0 * length(posLightSpace.xy - 0.5),0.0,1.0), 10.0));
	if (max(abs(posLightSpace.x - 0.5), abs(posLightSpace.y - 0.5)) > 0.5)
		distance_rate = 0.0;
	adjShadowStrengthCorrected = max(adjShadowStrength - mtsmoothstep(0.9, 1.1, posLightSpace.z),0.0);

	if (distance_rate > 1e-7) {

#ifdef COLORED_SHADOWS
		vec4 visibility;
		if (cosLight > 0.0 || normalLength < 1e-3)
			visibility = getShadowColor(
				ShadowMapSampler, posLightSpace.xy, posLightSpace.z,
				normalLength, perspFactor, f_shadowfar, f_textureresolution);
		else
			visibility = vec4(1.0, 0.0, 0.0, 0.0);
		shadow_int = visibility.r;
		shadow_color = visibility.gba;
#else
		if (cosLight > 0.0 || normalLength < 1e-3)
			shadow_int = getShadow(
				ShadowMapSampler, posLightSpace.xy, posLightSpace.z,
				normalLength, perspFactor, f_shadowfar, f_textureresolution);
		else
			shadow_int = 1.0;
#endif
		shadow_int *= distance_rate;
		shadow_int = clamp(shadow_int, 0.0, 1.0);

	}

	// turns out that nightRatio falls off much faster than
	// actual brightness of artificial light in relation to natual light.
	// Power ratio was measured on torches in MTG (brightness = 14).
	float adjusted_night_ratio = pow(max(0.0, nightRatio), 0.6);

	shadowUncorrected = shadow_int;

	// Apply self-shadowing when light falls at a narrow angle to the surface
	// Cosine of the cut-off angle.
	const float self_shadow_cutoff_cosine = 0.035;
	if (normalLength != 0 && cosLight < self_shadow_cutoff_cosine) {
		shadow_int = max(shadow_int, 1 - clamp(cosLight, 0.0, self_shadow_cutoff_cosine)/self_shadow_cutoff_cosine);
		shadow_color = mix(vec3(0.0), shadow_color, min(cosLight, self_shadow_cutoff_cosine)/self_shadow_cutoff_cosine);

#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES || MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS)
		// Prevents foliage from becoming insanely bright outside the shadow map.
		shadowUncorrected = mix(shadow_int, shadowUncorrected, clamp(distance_rate * 4.0 - 3.0, 0.0, 1.0));
#endif
	}

	shadow_int *= adjShadowStrengthCorrected;

	// calculate fragment color from components:
	return vec4(adjusted_night_ratio * color.rgb + // artificial light
			(1.0 - adjusted_night_ratio) * ( // natural light
					color.rgb * (1.0 - shadow_int * (1.0 - shadow_color) * (1.0 - shadow_tint)) +  // filtered texture color
					dayLight * shadow_color * shadow_int), color.a);                 // reflected filtered sunlight/moonlight
}
