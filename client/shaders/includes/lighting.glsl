uniform vec3 nightSkyColor;
uniform vec3 sunriseSkyColor;
uniform vec3 daySkyColor;
uniform vec3 sunsetSkyColor;
uniform vec3 ambientColor;

const vec3 blockColor = vec3(1.0, 0.85, 0.7);

vec3 getSkyColor(float timeOfDay)
{
    // Handle night period (including wrap-around from 0.825 to 0.18)
    float isNight = step(0.825, timeOfDay) + step(timeOfDay, 0.18);
    isNight = clamp(isNight, 0.0, 1.0);

    // Normalize timeOfDay for non-night periods
    float t = timeOfDay;
    t = t < 0.18 ? t + 1.0 : t;

    // Sunrise interpolation factors (shifted by -0.02)
    // [0.18;0.23] and [0.23;0.28]
    float sunrise1 = smoothstep(0.18, 0.23, t);
    float sunrise2 = 1.0 - smoothstep(0.23, 0.28, t);
    float sunriseFactor = sunrise1 * sunrise2;

    // Day factor (constant in [0.28-0.725])
    float dayFactor = step(0.28, t) * (1.0 - step(0.725, t));

    // Sunset interpolation factors (shifted forward by +0.025)
    // Old: [0.70;0.75] and [0.75;0.80]
    // New: [0.725;0.775] and [0.775;0.825]
    float sunset1 = smoothstep(0.725, 0.775, t);
    float sunset2 = 1.0 - smoothstep(0.775, 0.825, t);
    float sunsetFactor = sunset1 * sunset2;

    // Determine which period we're in (each period gets 1, others 0)
    // Sunrise period: [0.18;0.28]
    float isSunrise = step(0.18, t) * (1.0 - step(0.28, t));
    // Day period: [0.28;0.725]
    float isDay = step(0.28, t) * (1.0 - step(0.725, t));
    // Sunset period: [0.725;0.825]
    float isSunset = step(0.725, t) * (1.0 - step(0.825, t));

    // Apply smoothstep interpolation within sunrise and sunset periods
    // Sunrise peak at 0.23
    vec3 sunriseColor = mix(nightSkyColor, sunriseSkyColor, sunrise1) * (1.0 - step(0.23, t)) +
                        mix(sunriseSkyColor, daySkyColor, 1.0 - sunrise2) * step(0.23, t);
    sunriseColor = sunriseColor * isSunrise;

    // Sunset peak at 0.775 (was 0.75)
    vec3 sunsetColor = mix(daySkyColor, sunsetSkyColor, sunset1) * (1.0 - step(0.775, t)) +
                       mix(sunsetSkyColor, nightSkyColor, 1.0 - sunset2) * step(0.775, t);
    sunsetColor = sunsetColor * isSunset;

    vec3 dayColor = daySkyColor * isDay;
    vec3 nightColor = nightSkyColor * isNight;

    // Combine everything
    return nightColor + sunriseColor + dayColor + sunsetColor;
}

vec3 calculateLighting(float timeOfDay, float skyLight, float blockLight, float ao)
{
	vec3 skyColor = getSkyColor(timeOfDay);
	vec3 mixedColor = max(blockColor * blockLight, skyColor * skyLight);

	mixedColor += ambientColor;
	mixedColor *= ao;

	return mixedColor;
}
