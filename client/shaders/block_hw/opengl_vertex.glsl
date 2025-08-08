layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;
layout (location = 4) in int materialType;
layout (location = 5) in vec3 hwcolor;

#include <matrices>

layout (std140) uniform mShadowParams {
	// shadow texture
	sampler2D shadowMapSampler;
	// shadow uniforms
	vec3 lightDirection;
	float textureresolution;
	mat4 shadowViewProj;
	float shadowfar;
	float shadow_strength;
	float timeofday;
	vec4 cameraPos;
	float xyPerspectiveBias0;
	float xyPerspectiveBias1;
	float zPerspectiveBias;
	vec3 shadowTint;
}

// Color of the light emitted by the sun.
uniform vec3 mDayLight;

// The cameraOffset is the current center of the visible world.
uniform highp vec3 mCameraOffset;
uniform float mAnimationTimer;

out vec3 vNormal;
out vec3 vPosition;
// World position in the visible world (i.e. relative to the cameraOffset.)
// This can be used for many shader effects without loss of precision.
// If the absolute position is required it can be calculated with
// cameraOffset + worldPosition (for large coordinates the limits of float
// precision must be considered).
out vec3 vWorldPosition;
out lowp vec4 vColor;
out lowp vec3 vHWColor;
// The centroid keyword ensures that after interpolation the texture coordinates
// lie within the same bounds when MSAA is en- and disabled.
// This fixes the stripes problem with nearest-neighbor textures and MSAA.
#ifdef GL_ES
out mediump vec2 vTexCoord;
#else
centroid out vec2 vTexCoord;
#endif
#ifdef ENABLE_DYNAMIC_SHADOWS
	out float vCosLight;
	out float vNormalOffsetScale;
	out float vAdjShadowStrength;
	out float vFNormalLength;
	out vec3 vShadowPosition;
	out float vPerspectiveFactor;
#endif

out float vAreaEnableParallax;

out highp vec3 vEyeVec;
out float vNightRatio;
// Color of the light emitted by the light sources.
const vec3 artificialLight = vec3(1.04, 1.04, 1.04);
const float e = 2.718281828459;
const float BS = 10.0;

#ifdef ENABLE_DYNAMIC_SHADOWS

vec4 getRelativePosition(in vec4 position)
{
	vec2 l = position.xy - mShadowParams.cameraPos.xy;
	vec2 s = l / abs(l);
	s = (1.0 - s * mShadowParams.cameraPos.xy);
	l /= s;
	return vec4(l, s);
}

float getPerspectiveFactor(in vec4 relativePosition)
{
	float pDistance = length(relativePosition.xy);
	float pFactor = pDistance * mShadowParams.xyPerspectiveBias0 + mShadowParams.xyPerspectiveBias1;
	return pFactor;
}

vec4 applyPerspectiveDistortion(in vec4 position)
{
	vec4 l = getRelativePosition(position);
	float pFactor = getPerspectiveFactor(l);
	l.xy /= pFactor;
	position.xy = l.xy * l.zw + mShadowParams.cameraPos.xy;
	position.z *= mShadowParams.zPerspectiveBias;
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


float smoothCurve(float x)
{
	return x * x * (3.0 - 2.0 * x);
}


float triangleWave(float x)
{
	return abs(fract(x + 0.5) * 2.0 - 1.0);
}


float smoothTriangleWave(float x)
{
	return smoothCurve(triangleWave(x)) * 2.0 - 1.0;
}

//
// Simple, fast noise function.
// See: https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
//
vec4 perm(vec4 x)
{
	return mod(((x * 34.0) + 1.0) * x, 289.0);
}

float snoise(vec3 p)
{
	vec3 a = floor(p);
	vec3 d = p - a;
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

	return o4.y * d.y + o4.x * (1.0 - d.y);
}


void main(void)
{
	vTexCoord = uv;

	float disp_x;
	float disp_z;
	// OpenGL < 4.3 does not support continued preprocessor lines
	if (materialType == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES) || (materialType == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS) {
		vec4 pos2 = mMatrices.world * pos;
		float tOffset = (pos2.x + pos2.y) * 0.001 + pos2.z * 0.002;
		disp_x = (smoothTriangleWave(mAnimationTimer * 23.0 + tOffset) +
		smoothTriangleWave(mAnimationTimer * 11.0 + tOffset)) * 0.4;
		disp_z = (smoothTriangleWave(mAnimationTimer * 31.0 + tOffset) +
		smoothTriangleWave(mAnimationTimer * 29.0 + tOffset) +
		smoothTriangleWave(mAnimationTimer * 13.0 + tOffset)) * 0.5;
	}

	vec4 cpos = pos;
	if (materialType == MATERIAL_WAVING_LIQUID && ENABLE_WAVING_WATER) {
		// Generate waves with Perlin-type noise.
		// The constants are calibrated such that they roughly
		// correspond to the old sine waves.
		vec3 wavePos = (mMatrices.world * pos).xyz + cameraOffset;
		// The waves are slightly compressed along the z-axis to get
		// wave-fronts along the x-axis.
		wavePos.x /= WATER_WAVE_LENGTH * 3.0;
		wavePos.z /= WATER_WAVE_LENGTH * 2.0;
		wavePos.z += animationTimer * WATER_WAVE_SPEED * 10.0;
		cpos.y += (snoise(wavePos) - 1.0) * WATER_WAVE_HEIGHT * 5.0;
	}
	else if (materialType == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES) {
		cpos.x += disp_x;
		cpos.y += disp_z * 0.1;
		cpos.z += disp_z;
	}
	else if (materialType == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS) {
		if (varTexCoord.y < 0.05) {
			cpos.x += disp_x;
			cpos.z += disp_z;
		}
	}
	vWorldPosition = (mMatrices.world * cpos).xyz;
	gl_Position = mMatrices.worldViewProj * cpos;

	vPosition = gl_Position.xyz;
	vEyeVec = -(mMatrices.worldView * cpos).xyz;
#ifdef SECONDSTAGE
	normalPass = normalize((normal+1)/2);
#endif
	vNormal = normal;

	// Calculate color.
	vColor = color;
	// Red, green and blue components are pre-multiplied with
	// the brightness, so now we have to multiply these
	// colors with the color of the incoming light.
	// The pre-baked colors are halved to prevent overflow.
	// The alpha gives the ratio of sunlight in the incoming light.
	vNightRatio = 1.0 - vColor.a;
	vColor.rgb = vColor.rgb * (vColor.a * mDayLight.rgb +
		vNightRatio * artificialLight.rgb) * 2.0;
	vColor.a = 1.0;

	// Emphase blue a bit in darker places
	// See C++ implementation in mapblock_mesh.cpp final_color_blend()
	float brightness = (vColor.r + vColor.g + vColor.b) / 3.0;
	vColor.b += max(0.0, 0.021 - abs(0.2 * brightness - 0.021) +
		0.07 * brightness);

	vColor = clamp(vColor, 0.0, 1.0);

	vHWColor = hwcolor;

#ifdef ENABLE_DYNAMIC_SHADOWS
	if (mShadowParams.shadow_strength > 0.0) {
		vec4 shadow_pos;
	
		if (materialType == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS) {
			// The shadow shaders don't apply waving when creating the shadow-map.
			// We are using the not waved inVertexPosition to avoid ugly self-shadowing.
			shadow_pos = pos;
		}
		else
			shadow_pos = cpos;

		vec3 nNormal;
		vFNormalLength = length(vNormal);

		/* normalOffsetScale is in world coordinates (1/10th of a meter)
		   z_bias is in light space coordinates */
		float normalOffsetScale, z_bias;
		float pFactor = getPerspectiveFactor(getRelativePosition(mShadowParams.shadowViewProj * mMatrices.world * shadow_pos));
		if (vFNormalLength > 0.0) {
			nNormal = normalize(vNormal);
			vCosLight = max(1e-5, dot(nNormal, -mShadowParams.lightDirection));
			float sinLight = pow(1.0 - pow(vCosLight, 2.0), 0.5);
			normalOffsetScale = 2.0 * pFactor * pFactor * sinLight * min(mShadowParams.shadowfar, 500.0) /
					mShadowParams.xyPerspectiveBias1 / mShadowParams.textureresolution;
			z_bias = 1.0 * sinLight / vCosLight;
		}
		else {
			nNormal = vec3(0.0);
			vCosLight = clamp(dot(mShadowParams.lightDirection, normalize(vec3(mShadowParams.lightDirection.x, 0.0, mShadowParams.lightDirection.z))), 1e-2, 1.0);
			float sinLight = pow(1.0 - pow(vCosLight, 2.0), 0.5);
			normalOffsetScale = 0.0;
			z_bias = 3.6e3 * sinLight / vCosLight;
		}
		z_bias *= pFactor * pFactor / mShadowParams.textureresolution / mShadowParams.shadowfar;

		vShadowPosition = applyPerspectiveDistortion(mShadowParams.shadowViewProj * mMatrices.world * (shadow_pos + vec4(normalOffsetScale * nNormal, 0.0))).xyz;
		if (!ENABLE_TRANSLUCENT_FOLIAGE || materialType != TILE_MATERIAL_WAVING_LEAVES)
			vShadowPosition.z -= z_bias;

		perspective_factor = pFactor;

		if (mShadowParams.timeofday < 0.2) {
			vAdjShadowStrength = mShadowParams.shadow_strength * 0.5 *
				(1.0 - mtsmoothstep(0.18, 0.2, mShadowParams.timeofday));
		} else if (mShadowParams.timeofday >= 0.8) {
			vAdjShadowStrength = mShadowParams.shadow_strength * 0.5 *
				mtsmoothstep(0.8, 0.83, mShadowParams.timeofday);
		} else {
			vAdjShadowStrength = mShadowParams.shadow_strength *
				mtsmoothstep(0.20, 0.25, mShadowParams.timeofday) *
				(1.0 - mtsmoothstep(0.7, 0.8, mShadowParams.timeofday));
		}
	}
#endif
}
