uniform sampler2D baseTexture;

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
#ifdef GL_ES
in lowp vec4 varColor;
in lowp vec3 dayLight;
in mediump vec2 varTexCoord;
in float nightRatio;
#else
centroid in lowp vec4 varColor;
centroid in lowp vec3 dayLight;
centroid in vec2 varTexCoord;
centroid in float nightRatio;
#endif
in highp vec3 eyeVec;

#ifdef ENABLE_DYNAMIC_SHADOWS
#if (defined(ENABLE_WATER_REFLECTIONS) && MATERIAL_WATER_REFLECTIONS && ENABLE_WAVING_WATER)
vec4 perm(vec4 x)
{
	return mod(((x * 34.0) + 1.0) * x, 289.0);
}

// Corresponding gradient of snoise
vec3 gnoise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    vec3 dd = 6.0 * d * (1.0 - d);
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    vec4 dz1 = (o2 - o1) * dd.z;
    vec2 dz2 = dz1.yw * d.x + dz1.xz * (1.0 - d.x);

    vec2 dx = (o3.yw - o3.xz) * dd.x;

    return vec3(
        dx.y * d.y + dx.x * (1. - d.y),
        (o4.y - o4.x) * dd.y,
        dz2.y * d.y + dz2.x * (1. - d.y)
    );
}

vec2 wave_noise(vec3 p, float off) {
	return (gnoise(p + vec3(0.0, 0.0, off)) * 0.4 + gnoise(2.0 * p + vec3(0.0, off, off)) * 0.2 + gnoise(3.0 * p + vec3(0.0, off, off)) * 0.225 + gnoise(4.0 * p + vec3(-off, off, 0.0)) * 0.2).xz;
}
#endif
#endif

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

#ifdef ENABLE_DYNAMIC_SHADOWS
	// Fragment normal, can differ from vNormal which is derived from vertex normals.
	vec3 fNormal = vNormal;

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

		float shadow_uncorrected = shadow_int;

		// Apply self-shadowing when light falls at a narrow angle to the surface
		// Cosine of the cut-off angle.
		const float self_shadow_cutoff_cosine = 0.035;
		if (f_normal_length != 0 && cosLight < self_shadow_cutoff_cosine) {
			shadow_int = max(shadow_int, 1 - clamp(cosLight, 0.0, self_shadow_cutoff_cosine)/self_shadow_cutoff_cosine);
			shadow_color = mix(vec3(0.0), shadow_color, min(cosLight, self_shadow_cutoff_cosine)/self_shadow_cutoff_cosine);

#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES || MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS)
			// Prevents foliage from becoming insanely bright outside the shadow map.
			shadow_uncorrected = mix(shadow_int, shadow_uncorrected, clamp(distance_rate * 4.0 - 3.0, 0.0, 1.0));
#endif
		}

		shadow_int *= f_adj_shadow_strength;

		// calculate fragment color from components:
		col.rgb =
				adjusted_night_ratio * col.rgb + // artificial light
				(1.0 - adjusted_night_ratio) * ( // natural light
						col.rgb * (1.0 - shadow_int * (1.0 - shadow_color) * (1.0 - shadow_tint)) +  // filtered texture color
						dayLight * shadow_color * shadow_int);                 // reflected filtered sunlight/moonlight


		vec3 reflect_ray = -normalize(v_LightDirection - fNormal * dot(v_LightDirection, fNormal) * 2.0);

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
		reflect_ray = -normalize(v_LightDirection - fNormal * dot(v_LightDirection, fNormal) * 2.0);
		float fresnel_factor = dot(fNormal, viewVec);

		float brightness_factor = 1.0 - adjusted_night_ratio;

		// A little trig hack. We go from the dot product of viewVec and normal to the dot product of viewVec and tangent to apply a fresnel effect.
		fresnel_factor = clamp(pow(1.0 - fresnel_factor * fresnel_factor, 8.0), 0.0, 1.0) * 0.8 + 0.2;
		col.rgb *= 0.5;
		vec3 reflection_color = mix(vec3(max(fogColor.r, max(fogColor.g, fogColor.b))), fogColor.rgb, f_shadow_strength);

		// Sky reflection
		col.rgb += reflection_color * pow(fresnel_factor, 2.0) * 0.5 * brightness_factor;
		vec3 water_reflect_color = 12.0 * dayLight * fresnel_factor * mtsmoothstep(0.85, 0.9, pow(clamp(dot(reflect_ray, viewVec), 0.0, 1.0), 32.0)) * max(1.0 - shadow_uncorrected, 0.0);

		// This line exists to prevent ridiculously bright reflection colors.
		water_reflect_color /= clamp(max(water_reflect_color.r, max(water_reflect_color.g, water_reflect_color.b)) * 0.375, 1.0, 400.0);
		col.rgb += water_reflect_color * f_adj_shadow_strength * brightness_factor;
#endif

#if (defined(ENABLE_NODE_SPECULAR) && !MATERIAL_WATER_REFLECTIONS)
		// Apply specular to blocks.
		if (dot(v_LightDirection, vNormal) < 0.0) {
			float intensity = 2.0 * (1.0 - (base.r * varColor.r));
			const float specular_exponent = 5.0;
			const float fresnel_exponent =  4.0;

			col.rgb +=
				intensity * dayLight * (1.0 - nightRatio) * (1.0 - shadow_uncorrected) * f_adj_shadow_strength *
				pow(max(dot(reflect_ray, viewVec), 0.0), fresnel_exponent) * pow(1.0 - abs(dot(viewVec, fNormal)), specular_exponent);
		}
#endif

#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS || MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES) && defined(ENABLE_TRANSLUCENT_FOLIAGE)
		// Simulate translucent foliage.
		col.rgb += 4.0 * dayLight * base.rgb * normalize(base.rgb * varColor.rgb * varColor.rgb) * f_adj_shadow_strength * pow(max(-dot(v_LightDirection, viewVec), 0.0), 4.0) * max(1.0 - shadow_uncorrected, 0.0);
#endif
	}
#endif

	col = mixColorWithFog(col, eyeVec);
	col = vec4(col.rgb, base.a);

	outColor0 = col;
}
