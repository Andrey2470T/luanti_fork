// Color of the light emitted by the light sources.
const vec3 artificialLight = vec3(1.04, 1.04, 1.04);

// daynightRatio: 0-1000
vec3 getSunlightColor(float daynightRatio)
{
	float rg = daynight_ratio / 1000.0 - 0.04;
    float b = (0.98 * daynight_ratio) / 1000.0 + 0.078;

    return vec3(rg, rg, b);
}

vec4 finalLightColor(float daynightRatio, vec4 lightColor, out float nightRatio)
{
	vec3 dayLight = getSunlightColor(daynightRatio);

	vec4 finalColor = lightColor;
	// Red, green and blue components are pre-multiplied with
	// the brightness, so now we have to multiply these
	// colors with the color of the incoming light.
	// The pre-baked colors are halved to prevent overflow.
	// The alpha gives the ratio of sunlight in the incoming light.
	nightRatio = 1.0 - finalColor.a;
	finalColor.rgb = finalColor.rgb * (finalColor.a * dayLight +
		nightRatio * artificialLight) * 2.0;
	finalColor.a = 1.0;

	// Emphase blue a bit in darker places
	// See C++ implementation in mapblock_mesh.cpp final_color_blend()
	float brightness = (finalColor.r + finalColor.g + finalColor.b) / 3.0;
	finalColor.b += max(0.0, 0.021 - abs(0.2 * brightness - 0.021) +
		0.07 * brightness);

	finalColor = clamp(finalColor, 0.0, 1.0);

	return finalColor;
}
