uniform mat4 mWorld;
uniform float dayNightRatio;
uniform float animationTimer;
uniform lowp vec4 materialColor;

out vec3 vNormal;
out vec3 vPosition;
out vec3 worldPosition;
out lowp vec4 varColor;
out lowp vec3 dayLight;

#ifdef GL_ES
out mediump vec2 varTexCoord;
#else
centroid out vec2 varTexCoord;
#endif

#ifdef ENABLE_DYNAMIC_SHADOWS
	out float cosLight;
	out float adj_shadow_strength;
	out float f_normal_length;
	out vec3 shadow_position;
	out float perspective_factor;
#endif

out highp vec3 eyeVec;
out float nightRatio;
out float vIDiff;
const float e = 2.718281828459;
const float BS = 10.0;

float directional_ambient(vec3 normal)
{
	vec3 v = normal * normal;

	if (normal.y < 0.0)
		return dot(v, vec3(0.670820, 0.447213, 0.836660));

	return dot(v, vec3(0.670820, 1.000000, 0.836660));
}

void main(void)
{
	varTexCoord = (mTexture * vec4(inTexCoord0.xy, 1.0, 1.0)).st;
	gl_Position = mWorldViewProj * vec4(inPosition, 1.0);

	vPosition = gl_Position.xyz;
	vNormal = (mWorld * vec4(inNormal, 0.0)).xyz;
	worldPosition = (mWorld * vec4(inPosition, 1.0)).xyz;
	eyeVec = -(mWorldView * vec4(inPosition, 1.0)).xyz;

#if (MATERIAL_TYPE == TILE_MATERIAL_PLAIN) || (MATERIAL_TYPE == TILE_MATERIAL_PLAIN_ALPHA)
	vIDiff = 1.0;
#else
	// This is intentional comparison with zero without any margin.
	// If normal is not equal to zero exactly, then we assume it's a valid, just not normalized vector
	vIDiff = length(inNormal) == 0.0
		? 1.0
		: directional_ambient(normalize(inNormal));
#endif

	// Calculate color.
	vec4 color = inColor;
	color *= materialColor;
	dayLight = getSunlightColor(dayNightRatio);
	nightRatio = 1.0 - color.a;
	varColor = finalLightColor(dayLight, color);

#ifdef ENABLE_DYNAMIC_SHADOWS
	if (f_shadow_strength > 0.0) {
		vec3 nNormal = normalize(vNormal);
		f_normal_length = length(vNormal);

		/* normalOffsetScale is in world coordinates (1/10th of a meter)
		   z_bias is in light space coordinates */
		float normalOffsetScale, z_bias;
		float pFactor = getPerspectiveFactor(getRelativePosition(m_ShadowViewProj * mWorld * vec4(inPosition, 1.0)));
		if (f_normal_length > 0.0) {
			nNormal = normalize(vNormal);
			cosLight = max(1e-5, dot(nNormal, -v_LightDirection));
			float sinLight = pow(1.0 - pow(cosLight, 2.0), 0.5);
			normalOffsetScale = 0.1 * pFactor * pFactor * sinLight * min(f_shadowfar, 500.0) /
					xyPerspectiveBias1 / f_textureresolution;
			z_bias = 1e3 * sinLight / cosLight * (0.5 + f_textureresolution / 1024.0);
		}
		else {
			nNormal = vec3(0.0);
			cosLight = clamp(dot(v_LightDirection, normalize(vec3(v_LightDirection.x, 0.0, v_LightDirection.z))), 1e-2, 1.0);
			float sinLight = pow(1.0 - pow(cosLight, 2.0), 0.5);
			normalOffsetScale = 0.0;
			z_bias = 3.6e3 * sinLight / cosLight;
		}
		z_bias *= pFactor * pFactor / f_textureresolution / f_shadowfar;

		shadow_position = applyPerspectiveDistortion(m_ShadowViewProj * mWorld * (vec4(inPosition, 1.0) + vec4(normalOffsetScale * nNormal, 0.0))).xyz;
		shadow_position.z -= z_bias;
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
