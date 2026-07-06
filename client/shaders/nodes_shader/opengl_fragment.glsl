#include<fog>
#include<noise>
#include<blending>

// The texture saving the current single crack frame updated externally
#define crackFrameTexture texture1
uniform sampler2D crackFrameTexture;

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
	vec4 base = texelFetch(baseTexture, ivec2(uv.x, uv.y), 0);

	if (bool(hasCrack)) {
		vec4 crack = texture(crackFrameTexture, crackTexCoord);
		base = DoBlend(OVERLAY_BLEND_MODE, base, crack);
	}

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
	// Fragment normal, can differ from vNormal which is derived from vertex normals.
	vec3 fNormal = vNormal;

	vec3 reflect_ray = -normalize(lightDir - fNormal * dot(lightDir, fNormal) * 2.0);

	vec3 viewVec = normalize(worldPosition + cameraOffset - cameraPosition);

		// Water reflections
#if (defined(ENABLE_WATER_REFLECTIONS) && MATERIAL_WATER_REFLECTIONS && ENABLE_WAVING_WATER)
		vec3 wavePos = worldPosition * vec3(2.0, 0.0, 2.0);
		float off = animationTimer * WATER_WAVE_SPEED * 10.0;
		wavePos.x /= WATER_WAVE_LENGTH * 3.0;
		wavePos.z /= WATER_WAVE_LENGTH * 2.0;

		// This is an analogous method to the bumpmap, except we get the gradient information directly from gnoise.
		vec2 gradient = wave_noise(wavePos, off);
		fNormal = normalize(normalize(fNormal) + vec3(gradient.x, 0., gradient.y) * WATER_WAVE_HEIGHT * abs(fNormal.y) * 0.25);
		reflect_ray = -normalize(lightDir - fNormal * dot(lightDir, fNormal) * 2.0);
		float fresnel_factor = dot(fNormal, viewVec);

		float adjusted_night_ratio = pow(max(0.0, nightRatio), 0.6);
		float brightness_factor = 1.0 - adjusted_night_ratio;

		// A little trig hack. We go from the dot product of viewVec and normal to the dot product of viewVec and tangent to apply a fresnel effect.
		fresnel_factor = clamp(pow(1.0 - fresnel_factor * fresnel_factor, 8.0), 0.0, 1.0) * 0.8 + 0.2;
		col.rgb *= 0.5;
		vec3 reflection_color = mix(vec3(max(fogColor.r, max(fogColor.g, fogColor.b))), fogColor.rgb, shadow_strength);

		// Sky reflection
		col.rgb += reflection_color * pow(fresnel_factor, 2.0) * 0.5 * brightness_factor;
		vec3 water_reflect_color = 12.0 * dayLight * fresnel_factor * mtsmoothstep(0.85, 0.9, pow(clamp(dot(reflect_ray, viewVec), 0.0, 1.0), 32.0)) * max(1.0 - shadow_uncorrected, 0.0);

		// This line exists to prevent ridiculously bright reflection colors.
		water_reflect_color /= clamp(max(water_reflect_color.r, max(water_reflect_color.g, water_reflect_color.b)) * 0.375, 1.0, 400.0);
		col.rgb += water_reflect_color * f_adj_shadow_strength * brightness_factor;
#endif

#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS || MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES) && defined(ENABLE_TRANSLUCENT_FOLIAGE)
		// Simulate translucent foliage.
		col.rgb += 4.0 * dayLight * base.rgb * normalize(base.rgb * varColor.rgb * varColor.rgb) * f_adj_shadow_strength * pow(max(-dot(lightDir, viewVec), 0.0), 4.0) * max(1.0 - shadow_uncorrected, 0.0);
#endif

	col = mixColorWithFog(col, eyeVec);
	col = vec4(col.rgb, base.a);

#ifndef ENABLE_BLOOM
	outputColor(col);
#else
	outputColor(col, vec4(col.rgb * emissionLight, 1.0));
#endif
}
