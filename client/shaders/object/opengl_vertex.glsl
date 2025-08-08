layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;
layout (location = 4) in int materialType;
layout (location = 5) in vec2i bones;	// packed bones IDs (8 u8 numbers)
layout (location = 6) in vec2i weights; // packed weights (8 u8 numbers)

#extension GL_EXT_gpu_shader4 : enable // for bitwise operators

#define BONES_MAX 128
#define BONES_IDS_MAX 8 // per a vertex

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
};

// Absolute bones transformations
uniform mat4 mBonesTransforms[BONES_MAX];

// Whether animate the normals
uniform int mAnimateNormals;

uniform vec3 mDayLight;
uniform float mAnimationTimer;
uniform lowp vec4 mMaterialColor;

out vec3 vNormal;
out vec3 vPosition;
out vec3 vWorldPosition;
out lowp vec4 vColor;
#ifdef GL_ES
out mediump vec2 vUV0;
#else
centroid out vec2 vUV0;
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
// Color of the light emitted by the light sources.
const vec3 artificialLight = vec3(1.04, 1.04, 1.04);
out float vIDiff;
const float e = 2.718281828459;
const float BS = 10.0;

#ifdef ENABLE_DYNAMIC_SHADOWS

vec4 getRelativePosition(in vec4 position)
{
	vec2 l = position.xy - mCameraPos.xy;
	vec2 s = l / abs(l);
	s = (1.0 - s * mCameraPos.xy);
	l /= s;
	return vec4(l, s);
}

float getPerspectiveFactor(in vec4 relativePosition)
{
	float pDistance = length(relativePosition.xy);
	float pFactor = pDistance * mmXYPerspectiveBias0 + mmXYPerspectiveBias1;
	return pFactor;
}

vec4 applyPerspectiveDistortion(in vec4 position)
{
	vec4 l = getRelativePosition(position);
	float pFactor = getPerspectiveFactor(l);
	l.xy /= pFactor;
	position.xy = l.xy * l.zw + mCameraPos.xy;
	position.z *= mZPerspectiveBias;
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
	vUV0 = (mMatrices.texture0 * vec4(uv.xy, 1.0, 1.0)).st;

	// Calculate weighted transformations from each affected bone for the current frame
	vec3 skinnedPos = vec3(0.0);
	vec3 skinnedNormal = normal;

	for (int i = 0; i < BONES_IDS_MAX; i++) {
		int shift_n = i - (i % 4) * 4;

		// Extract bone ID from bones
		int bone_n = bones[i % 4];
		int bone_id = clamp((bone_n >> (shift_n * 8)) & 0xffffffff, 0, BONES_MAX-1);

		// Extract weight from weights
		int weight_n = weights[i % 4];
		float weight = float((weight_n >> (shift_n * 8)) & 0xffffffff) / 127.0;

		skinnedPos += weight * mBonesTransforms[bone_id] * vec4(skinnedPos, 1.0);

		if (mAnimateNormals)
			skinnedNormal += weight * mBonesTransforms[bone_id] * vec4(skinnedNormal, 0.0);
	}
	gl_Position = mMatrices.worldViewProj * vec4(skinnedPos, 1.0);

	vPosition = gl_Position.xyz;
	vNormal = (mMatrices.world * vec4(skinnedNormal, 0.0)).xyz;
	vWorldPosition = (mMatrices.world * vec4(skinnedPos, 1.0)).xyz;
	vEyeVec = -(mMatrices.worldView * vec4(skinnedPos, 1.0)).xyz;

	if (materialType == TILE_MATERIAL_PLAIN) || (materialType == TILE_MATERIAL_PLAIN_ALPHA)
		vIDiff = 1.0;
	else {
		// This is intentional comparison with zero without any margin.
		// If normal is not equal to zero exactly, then we assume it's a valid, just not normalized vector
		vIDiff = length(skinnedNormal) == 0.0
			? 1.0
			: directional_ambient(normalize(skinnedNormal));
	}

	vColor = color;

	vColor *= mMaterialColor;

	// The alpha gives the ratio of sunlight in the incoming light.
	vvNightRatio = 1.0 - vColor.a;
	vColor.rgb = vColor.rgb * (vColor.a * mDayLight.rgb +
		vvNightRatio * artificialLight.rgb) * 2.0;
	vColor.a = 1.0;

	// Emphase blue a bit in darker places
	// See C++ implementation in mapblock_mesh.cpp final_color_blend()
	float brightness = (vColor.r + vColor.g + vColor.b) / 3.0;
	vColor.b += max(0.0, 0.021 - abs(0.2 * brightness - 0.021) +
		0.07 * brightness);

	vColor = clamp(vColor, 0.0, 1.0);


#ifdef ENABLE_DYNAMIC_SHADOWS
	if (mShadowStrength > 0.0) {
		vec3 nNormal = normalize(vNormal);
		vNormalLength = length(vNormal);

		/* normalOffsetScale is in world coordinates (1/10th of a meter)
		   z_bias is in light space coordinates */
		float normalOffsetScale, z_bias;
		float pFactor = getPerspectiveFactor(getRelativePosition(mShadowViewProj * skinnedPos));
		if (vNormalLength > 0.0) {
			nNormal = normalize(vNormal);
			vCosLight = max(1e-5, dot(nNormal, -mLightDirection));
			float sinLight = pow(1.0 - pow(vCosLight, 2.0), 0.5);
			normalOffsetScale = 0.1 * pFactor * pFactor * sinLight * min(mShadowFar, 500.0) /
					mmXYPerspectiveBias1 / mTextureResolution;
			z_bias = 1e3 * sinLight / vCosLight * (0.5 + mTextureResolution / 1024.0);
		}
		else {
			nNormal = vec3(0.0);
			vCosLight = clamp(dot(mLightDirection, normalize(vec3(mLightDirection.x, 0.0, mLightDirection.z))), 1e-2, 1.0);
			float sinLight = pow(1.0 - pow(vCosLight, 2.0), 0.5);
			normalOffsetScale = 0.0;
			z_bias = 3.6e3 * sinLight / vCosLight;
		}
		z_bias *= pFactor * pFactor / mTextureResolution / mShadowFar;

		// Possible breakage: the skinned position is in the absolute coords, not in the relative
		vShadowPosition = applyPerspectiveDistortion(mShadowViewProj * vec4(skinnedPos + normalOffsetScale * nNormal, 1.0)).xyz;
		//vShadowPosition = applyPerspectiveDistortion(mShadowViewProj * mMatrices.world * (vec4(pos, 1.0) + vec4(normalOffsetScale * nNormal, 0.0))).xyz;
		vShadowPosition.z -= z_bias;
		vPerspectiveFactor = pFactor;

		if (mTimeOfDay < 0.2) {
			vAdjShadowStrength = mShadowStrength * 0.5 *
				(1.0 - mtsmoothstep(0.18, 0.2, mTimeOfDay));
		} else if (mTimeOfDay >= 0.8) {
			vAdjShadowStrength = mShadowStrength * 0.5 *
				mtsmoothstep(0.8, 0.83, mTimeOfDay);
		} else {
			vAdjShadowStrength = mShadowStrength *
				mtsmoothstep(0.20, 0.25, mTimeOfDay) *
				(1.0 - mtsmoothstep(0.7, 0.8, mTimeOfDay));
		}
	}
#endif
}
