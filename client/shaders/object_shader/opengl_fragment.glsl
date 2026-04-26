uniform sampler2D baseTexture;

// The cameraOffset is the current center of the visible world.
uniform highp vec3 cameraOffset;
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
in lowp vec4 varColor;
in lowp vec3 dayLight;

#ifdef GL_ES
in mediump vec2 varTexCoord;
#else
centroid in vec2 varTexCoord;
#endif
in highp vec3 eyeVec;
in float nightRatio;

in float vIDiff;

void main(void)
{
	vec3 color;
	vec2 uv = varTexCoord.st;

	vec4 base = texture2D(baseTexture, uv).rgba;
	// If alpha is zero, we can just discard the pixel. This fixes transparency
	// on GPUs like GC7000L, where GL_ALPHA_TEST is not implemented in mesa,
	// and also on GLES 2, where GL_ALPHA_TEST is missing entirely.
#ifdef USE_DISCARD
	if (base.a == 0.0)
		discard;
#endif
#ifdef USE_DISCARD_REF
	if (base.a < 0.5)
		discard;
#endif

	color = base.rgb;
	vec4 col = vec4(color.rgb * varColor.rgb, 1.0);
	col.rgb *= vIDiff;

#ifdef ENABLE_DYNAMIC_SHADOWS
	if (f_shadow_strength > 0.0) {
		float shadow_int = 0.0;
		vec3 shadow_color = vec3(0.0, 0.0, 0.0);
		vec3 posLightSpace = getLightSpacePosition(shadow_position);

		float distance_rate = (1.0 - pow(clamp(2.0 * length(posLightSpace.xy - 0.5),0.0,1.0), 10.0));
		if (max(abs(posLightSpace.x - 0.5), abs(posLightSpace.y - 0.5)) > 0.5)
			distance_rate = 0.0;
		float f_adj_shadow_strength = max(adj_shadow_strength - mtsmoothstep(0.9, 1.1, posLightSpace.z),0.0);

		if (distance_rate > 1e-7) {

#ifdef COLORED_SHADOWS
			vec4 visibility;
			if (cosLight > 0.0 || f_normal_length < 1e-3)
				visibility = getShadowColor(
					ShadowMapSampler, posLightSpace.xy, posLightSpace.z,
					f_normal_length, perspective_factor, f_shadowfar, f_textureresolution);
			else
				visibility = vec4(1.0, 0.0, 0.0, 0.0);
			shadow_int = visibility.r;
			shadow_color = visibility.gba;
#else
			if (cosLight > 0.0 || f_normal_length < 1e-3)
				shadow_int = getShadow(
					ShadowMapSampler, posLightSpace.xy, posLightSpace.z,
					f_normal_length, perspective_factor, f_shadowfar, f_textureresolution);
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

		// Apply self-shadowing when light falls at a narrow angle to the surface
		// Cosine of the cut-off angle.
		const float self_shadow_cutoff_cosine = 0.14;
		if (f_normal_length != 0 && cosLight < self_shadow_cutoff_cosine) {
			shadow_int = max(shadow_int, 1 - clamp(cosLight, 0.0, self_shadow_cutoff_cosine)/self_shadow_cutoff_cosine);
			shadow_color = mix(vec3(0.0), shadow_color, min(cosLight, self_shadow_cutoff_cosine)/self_shadow_cutoff_cosine);
		}

		shadow_int *= f_adj_shadow_strength;

		// calculate fragment color from components:
		col.rgb =
				adjusted_night_ratio * col.rgb + // artificial light
				(1.0 - adjusted_night_ratio) * ( // natural light
						col.rgb * (1.0 - shadow_int * (1.0 - shadow_color) * (1.0 - shadow_tint)) +  // filtered texture color
						dayLight * shadow_color * shadow_int);                 // reflected filtered sunlight/moonlight
	}
#endif

	col = mixColorWithFog(col, eyeVec);
	col = vec4(col.rgb, base.a);

	outColor0 = col;
}
