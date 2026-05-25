#include<common>
#include<lighting>
#include<noise>

uniform mat4 mWorld;
uniform float timeOfDay;

// The cameraOffset is the current center of the visible world.
uniform highp vec3 cameraOffset;
uniform float animationTimer;

out vec3 vNormal;
out vec3 vPosition;
// World position in the visible world (i.e. relative to the cameraOffset.)
// This can be used for many shader effects without loss of precision.
// If the absolute position is required it can be calculated with
// cameraOffset + worldPosition (for large coordinates the limits of float
// precision must be considered).
out vec3 worldPosition;
// The centroid keyword ensures that after interpolation the texture coordinates
// lie within the same bounds when MSAA is en- and disabled.
// This fixes the stripes problem with nearest-neighbor textures and MSAA.
#ifdef GL_ES
out lowp vec3 varColor;
out lowp vec3 dayLight;
out mediump vec2 varTexCoord;
out float nightRatio;
#else
centroid out lowp vec3 varColor;
centroid out lowp vec3 dayLight;
centroid out vec2 varTexCoord;
centroid out float nightRatio;
#endif
#ifdef ENABLE_DYNAMIC_SHADOWS
	out float cosLight;
	out float normalOffsetScale;
	out float adj_shadow_strength;
	out float f_normal_length;
	out vec3 shadow_position;
	out float perspective_factor;
#endif

out highp vec3 eyeVec;
out vec3 hwColor;

void main(void)
{
	varTexCoord = inTexCoord0.st;

	float disp_x;
	float disp_z;
// OpenGL < 4.3 does not support continued preprocessor lines
#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES) || (MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS)
	vec4 pos2 = mWorld * vec4(inPosition, 1.0);
	float tOffset = (pos2.x + pos2.y) * 0.001 + pos2.z * 0.002;
	disp_x = (smoothTriangleWave(animationTimer * 23.0 + tOffset) +
		smoothTriangleWave(animationTimer * 11.0 + tOffset)) * 0.4;
	disp_z = (smoothTriangleWave(animationTimer * 31.0 + tOffset) +
		smoothTriangleWave(animationTimer * 29.0 + tOffset) +
		smoothTriangleWave(animationTimer * 13.0 + tOffset)) * 0.5;
#endif

	vec4 pos = vec4(inPosition, 1.0);
#if MATERIAL_WAVING_LIQUID && ENABLE_WAVING_WATER
	// Generate waves with Perlin-type noise.
	// The constants are calibrated such that they roughly
	// correspond to the old sine waves.
	vec3 wavePos = (mWorld * pos).xyz + cameraOffset;
	// The waves are slightly compressed along the z-axis to get
	// wave-fronts along the x-axis.
	wavePos.x /= WATER_WAVE_LENGTH * 3.0;
	wavePos.z /= WATER_WAVE_LENGTH * 2.0;
	wavePos.z += animationTimer * WATER_WAVE_SPEED * 10.0;
	pos.y += (snoise(wavePos) - 1.0) * WATER_WAVE_HEIGHT * 5.0;
#elif MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES
	pos.x += disp_x;
	pos.y += disp_z * 0.1;
	pos.z += disp_z;
#elif MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS
	if (varTexCoord.y < 0.05) {
		pos.x += disp_x;
		pos.z += disp_z;
	}
#endif
	worldPosition = (mWorld * pos).xyz;
	gl_Position = mWorldViewProj * pos;

	vPosition = gl_Position.xyz;
	eyeVec = -(mWorldView * pos).xyz;
#ifdef SECONDSTAGE
	normalPass = normalize((inNormal+1)/2);
#endif
	vNormal = inNormal;

	// Calculate color.
	vec4 color = inColor;
	float skyLight = color.r;
	float blockLight = color.g;
	float ao = color.b;
	float sum = float(max(skyLight + blockLight, 0));
	nightRatio = 1.0 - (skyLight / sum);
	dayLight = getSkyColor(timeOfDay);
	varColor = calculateLighting(skyLight, blockLight, timeOfDay, ao);

	hwColor = inAux / 255.0;


#ifdef ENABLE_DYNAMIC_SHADOWS
	if (f_shadow_strength > 0.0) {
#if MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS
		// The shadow shaders don't apply waving when creating the shadow-map.
		// We are using the not waved inPosition to avoid ugly self-shadowing.
		vec4 shadow_pos = vec4(inPosition, 1.0);
#else
		vec4 shadow_pos = pos;
#endif
		vec3 nNormal;
		f_normal_length = length(vNormal);

		/* normalOffsetScale is in world coordinates (1/10th of a meter)
		   z_bias is in light space coordinates */
		float normalOffsetScale, z_bias;
		float pFactor = getPerspectiveFactor(getRelativePosition(m_ShadowViewProj * mWorld * shadow_pos));
		if (f_normal_length > 0.0) {
			nNormal = normalize(vNormal);
			cosLight = max(1e-5, dot(nNormal, -v_LightDirection));
			float sinLight = pow(1.0 - pow(cosLight, 2.0), 0.5);
			normalOffsetScale = 2.0 * pFactor * pFactor * sinLight * min(f_shadowfar, 500.0) /
					xyPerspectiveBias1 / f_textureresolution;
			z_bias = 1.0 * sinLight / cosLight;
		}
		else {
			nNormal = vec3(0.0);
			cosLight = clamp(dot(v_LightDirection, normalize(vec3(v_LightDirection.x, 0.0, v_LightDirection.z))), 1e-2, 1.0);
			float sinLight = pow(1.0 - pow(cosLight, 2.0), 0.5);
			normalOffsetScale = 0.0;
			z_bias = 3.6e3 * sinLight / cosLight;
		}
		z_bias *= pFactor * pFactor / f_textureresolution / f_shadowfar;

		shadow_position = applyPerspectiveDistortion(m_ShadowViewProj * mWorld * (shadow_pos + vec4(normalOffsetScale * nNormal, 0.0))).xyz;
#if !defined(ENABLE_TRANSLUCENT_FOLIAGE) || MATERIAL_TYPE != TILE_MATERIAL_WAVING_LEAVES
		shadow_position.z -= z_bias;
#endif
		perspective_factor = pFactor;

		if (f_timeofday < 0.2) {
			adj_shadow_strength = f_shadow_strength * 0.5 *
				(1.0 - mtsmoothstep(0.18, 0.2, f_timeofday));
		} else if (f_timeofday >= 0.8) {
			adj_shadow_strength = f_shadow_strength * 0.5 *
				mtsmoothstep(0.8, 0.83, f_timeofday);
		} else {
			adj_shadow_strength = f_shadow_strength *
				mtsmoothstep(0.20, 0.25, f_timeofday) *
				(1.0 - mtsmoothstep(0.7, 0.8, f_timeofday));
		}
	}
#endif
}
