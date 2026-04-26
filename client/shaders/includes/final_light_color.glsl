// Color of the light emitted by the light sources.
const vec3 artificialLight = vec3(1.04, 1.04, 1.04);

// daynightRatio: 0-1000
vec3 getSunlightColor(float daynightRatio)
{
	float rg = daynightRatio / 1000.0 - 0.04;
    float b = (0.98 * daynightRatio) / 1000.0 + 0.078;

    return vec3(rg, rg, b);
}

vec4 finalLightColor(vec3 dayLight, vec4 lightColor)
{
    // Red, green and blue components are pre-multiplied with
    // the brightness, so now we have to multiply these
    // colors with the color of the incoming light.
    // The pre-baked colors are halved to prevent overflow.
    // The alpha gives the ratio of sunlight in the incoming light.
    float nightRatio = 1.0 - lightColor.a;
    lightColor.rgb = lightColor.rgb * (lightColor.a * dayLight +
            nightRatio * artificialLight) * 2.0;
    lightColor.a = 1.0;

    // Emphase blue a bit in darker places
    // See C++ implementation in mapblock_mesh.cpp final_color_blend()
    float brightness = (lightColor.r + lightColor.g + lightColor.b) / 3.0;
    lightColor.b += max(0.0, 0.021 - abs(0.2 * brightness - 0.021) +
            0.07 * brightness);

    lightColor = clamp(lightColor, 0.0, 1.0);

    return lightColor;
}
