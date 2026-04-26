float shadowCutoff(float x) {
	#if defined(ENABLE_TRANSLUCENT_FOLIAGE) && MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES
		return mtsmoothstep(0.0, 0.002, x);
	#else
		return step(0.0, x);
	#endif
}

#ifdef COLORED_SHADOWS
vec4 getHardShadowColor(sampler2D shadowsampler, vec2 smTexCoord, float realDistance)
{
	vec4 texDepth = texture2D(shadowsampler, smTexCoord.xy).rgba;

	float visibility = shadowCutoff(realDistance - texDepth.r);
	vec4 result = vec4(visibility, vec3(0.0,0.0,0.0));
	if (visibility < 0.1) {
		visibility = shadowCutoff(realDistance - texDepth.b);
		result = vec4(visibility, unpackColor(texDepth.a));
	}
	return result;
}

float getHardShadowDepth(sampler2D shadowsampler, vec2 smTexCoord, float realDistance)
{
	vec4 texDepth = texture2D(shadowsampler, smTexCoord.xy);
	float depth = max(realDistance - texDepth.r, realDistance - texDepth.b);
	return depth;
}
#else
float getHardShadow(sampler2D shadowsampler, vec2 smTexCoord, float realDistance)
{
	float texDepth = texture2D(shadowsampler, smTexCoord.xy).r;
	float visibility = shadowCutoff(realDistance - texDepth);
	return visibility;
}

float getHardShadowDepth(sampler2D shadowsampler, vec2 smTexCoord, float realDistance)
{
	float texDepth = texture2D(shadowsampler, smTexCoord.xy).r;
	float depth = realDistance - texDepth;
	return depth;
}
#endif
