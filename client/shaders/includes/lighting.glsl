vec3 getSkyColor(float timeOfDay)
{
	// Hardcoded sky colors in the 4 time points:
	// 1. midnight:	 0
	// 2. morning:	 0.25
	// 3. noon:		 0.5
	// 4. evening:	 0.75
	vec3 midnight = vec3(0.04, 0.04, 0.12);   // dark-blue
	vec3 sunrise  = vec3(0.80, 0.35, 0.20);   // pink-orange
	vec3 noon   =   vec3(1.00, 0.98, 0.95);   // white with blueish tint
	vec3 sunset   = vec3(0.85, 0.30, 0.15);   // orange-red

	vec3 color;
	if (timeOfDay < 0.25) {
		// Midnight -> Sunrise
		float t = timeOfDay / 0.25;
		color = mix(midnight, sunrise, t);
	} else if (timeOfDay < 0.5) {
		// Sunrise -> Noon
		float t = (timeOfDay - 0.25) / 0.25;
		color = mix(sunrise, noon, t);
	} else if (timeOfDay < 0.75) {
		// Noon -> Sunset
		float t = (timeOfDay - 0.5) / 0.25;
		color = mix(noon, sunset, t);
	} else {
		// Sunset -> Midnight
		float t = (timeOfDay - 0.75) / 0.25;
		color = mix(sunset, midnight, t);
	}

	return color;
}

vec3 calculateLighting(int skyLight, int blockLight, float timeOfDay)
{
	// 1. Normalizing the light values (0-15 -> 0.0-1.0)
	float blockIntensity = float(blockLight) / 15.0;
	float skyIntensity   = float(skyLight)   / 15.0;

	// 2. Calculate the light colors (the block one is hardcoded)
	vec3 blockColor = vec3(1.0, 0.65, 0.4);
	vec3 skyColor = getSkyColor(timeOfDay);

	// 3. Exponential intensity attenuation
	float maxLevel = max(float(blockLight), float(skyLight));
	float attenuation = pow(0.8, 15.0 - maxLevel);

	// 4. Normalized blending the light colors
	float totalIntensity = blockIntensity + skyIntensity;

	vec3 mixedColor;// = max(blockColor * blockIntensity, skyColor * skyIntensity);
	if (totalIntensity > 0.001) {
		mixedColor = (blockColor * blockIntensity + skyColor * skyIntensity) / totalIntensity;
	} else {
		mixedColor = vec3(0.0); // full darkness
	}

	// 5. Multiplying the mixed color with the intensity and attenuation
	float finalBrightness = min(totalIntensity, 1.0) * attenuation; // min(totalIntensity, 1.0) is to prevent light overflow
	vec3 finalColor = mixedColor * finalBrightness;

	// 6. Ambient light adding (hardcoded)
	vec3 ambientColor = vec3(0.05);
	finalColor = max(finalColor, ambientColor);

	return finalColor;
}
