vec3 getSkyColor(float timeOfDay)
{
    vec3 night = vec3(0.04, 0.04, 0.12);
    vec3 sunrise = vec3(0.80, 0.35, 0.20);
    vec3 day = vec3(1.00, 0.98, 0.95);
    vec3 sunset = vec3(0.85, 0.30, 0.15);

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
    vec3 sunriseColor = mix(night, sunrise, sunrise1) * (1.0 - step(0.23, t)) +
                        mix(sunrise, day, 1.0 - sunrise2) * step(0.23, t);
    sunriseColor = sunriseColor * isSunrise;

    // Sunset peak at 0.775 (was 0.75)
    vec3 sunsetColor = mix(day, sunset, sunset1) * (1.0 - step(0.775, t)) +
                       mix(sunset, night, 1.0 - sunset2) * step(0.775, t);
    sunsetColor = sunsetColor * isSunset;

    vec3 dayColor = day * isDay;
    vec3 nightColor = night * isNight;

    // Combine everything
    return nightColor + sunriseColor + dayColor + sunsetColor;
}

vec3 calculateLighting(float skyLight, float blockLight, float timeOfDay, float ao)
{
	// 1. Calculate the light colors (the block one is hardcoded)
	vec3 blockColor = vec3(1.0, 0.65, 0.4);
	vec3 skyColor = getSkyColor(timeOfDay);

	// 2. Blending the light colors
	vec3 mixedColor = blockColor * blockLight + skyColor * skyLight;

	// 3. Ambient light adding (hardcoded)
	vec3 ambientColor = vec3(0.5);
	mixedColor += ambientColor;

	// 4. Apply ambient occlusion
	mixedColor *= ao;

	return mixedColor;
}
