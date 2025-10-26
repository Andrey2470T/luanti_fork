layout (std140) uniform mShadowParams {
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
} ShadowParams;
