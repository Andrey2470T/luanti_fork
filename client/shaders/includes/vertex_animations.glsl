uniform float animationTimer;

// The cameraOffset is the current center of the visible world.
uniform highp vec3 cameraOffset;

#if ENABLE_WAVING_LEAVES
void animateLeavesVertex(in vec3 worldPos, inout vec4 localPos)
{
	float tOffset = (worldPos.x + worldPos.y) * 0.001 + worldPos.z * 0.002;
	float disp_x = (smoothTriangleWave(animationTimer * 23.0 + tOffset) +
		smoothTriangleWave(animationTimer * 11.0 + tOffset)) * 0.4;
	float disp_z = (smoothTriangleWave(animationTimer * 31.0 + tOffset) +
		smoothTriangleWave(animationTimer * 29.0 + tOffset) +
		smoothTriangleWave(animationTimer * 13.0 + tOffset)) * 0.5;

	localPos.x += disp_x;
	localPos.y += disp_z * 0.1;
	localPos.z += disp_z;
}
#endif

#if ENABLE_WAVING_PLANTS
void animatePlantVertex(in vec3 worldPos, inout vec4 localPos)
{
	float tOffset = (worldPos.x + worldPos.y) * 0.001 + worldPos.z * 0.002;
	float disp_x = (smoothTriangleWave(animationTimer * 23.0 + tOffset) +
		smoothTriangleWave(animationTimer * 11.0 + tOffset)) * 0.4;
	float disp_z = (smoothTriangleWave(animationTimer * 31.0 + tOffset) +
		smoothTriangleWave(animationTimer * 29.0 + tOffset) +
		smoothTriangleWave(animationTimer * 13.0 + tOffset)) * 0.5;

	//if (texCoord.y < 0.05) {
		localPos.x += disp_x;
		localPos.z += disp_z;
	//}
}
#endif

#if ENABLE_WAVING_WATER
void animateWaterVertex(in vec3 worldPos, inout vec4 localPos)
{
	// Generate waves with Perlin-type noise.
	// The constants are calibrated such that they roughly
	// correspond to the old sine waves.
	vec3 wavePos = worldPos + cameraOffset;
	// The waves are slightly compressed along the z-axis to get
	// wave-fronts along the x-axis.
	wavePos.x /= WATER_WAVE_LENGTH * 3.0;
	wavePos.z /= WATER_WAVE_LENGTH * 2.0;
	wavePos.z += animationTimer * WATER_WAVE_SPEED * 10.0;
	localPos.y += (snoise(wavePos) - 1.0) * WATER_WAVE_HEIGHT * 5.0;
}
#endif
