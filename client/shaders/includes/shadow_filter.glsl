#if SHADOW_FILTER == 2
	#define PCFBOUND 2.0 // 5x5
	#define PCFSAMPLES 25
#elif SHADOW_FILTER == 1
	#define PCFBOUND 1.0 // 3x3
	#define PCFSAMPLES 9
#else
	#define PCFBOUND 0.0
	#define PCFSAMPLES 1
#endif

#define BASEFILTERRADIUS 1.0

float getPenumbraRadius(
	sampler2D shadowsampler, vec2 smTexCoord, float realDistance,
	float normal_length, float perspective_factor, float shadowfar, float textureresolution)
{
	if (PCFBOUND == 0.0 || SOFTSHADOWRADIUS <= 0.0)
		return 0.0;

	float depth = getHardShadowDepth(shadowsampler, smTexCoord.xy, realDistance);
	float sharpness_factor = 1.0;
	float depth_to_blur = shadowfar / SOFTSHADOWRADIUS / xyPerspectiveBias0;

	if (depth > 0.0 && normal_length > 0.0)
		sharpness_factor = clamp(5.0 * depth * depth_to_blur, 0.0, 1.0);

	float world_to_texture = xyPerspectiveBias1 / perspective_factor / perspective_factor
			* textureresolution / 2.0 / shadowfar;
	float world_radius = 0.2;

	return max(BASEFILTERRADIUS * textureresolution / 4096.0,
	       sharpness_factor * world_radius * world_to_texture * SOFTSHADOWRADIUS);
}

#ifdef POISSON_FILTER
const vec2[64] poissonDisk = vec2[64](
	vec2(0.170019, -0.040254),
	vec2(-0.299417, 0.791925),
	vec2(0.645680, 0.493210),
	vec2(-0.651784, 0.717887),
	vec2(0.421003, 0.027070),
	vec2(-0.817194, -0.271096),
	vec2(-0.705374, -0.668203),
	vec2(0.977050, -0.108615),
	vec2(0.063326, 0.142369),
	vec2(0.203528, 0.214331),
	vec2(-0.667531, 0.326090),
	vec2(-0.098422, -0.295755),
	vec2(-0.885922, 0.215369),
	vec2(0.566637, 0.605213),
	vec2(0.039766, -0.396100),
	vec2(0.751946, 0.453352),
	vec2(0.078707, -0.715323),
	vec2(-0.075838, -0.529344),
	vec2(0.724479, -0.580798),
	vec2(0.222999, -0.215125),
	vec2(-0.467574, -0.405438),
	vec2(-0.248268, -0.814753),
	vec2(0.354411, -0.887570),
	vec2(0.175817, 0.382366),
	vec2(0.487472, -0.063082),
	vec2(0.355476, 0.025357),
	vec2(-0.084078, 0.898312),
	vec2(0.488876, -0.783441),
	vec2(0.470016, 0.217933),
	vec2(-0.696890, -0.549791),
	vec2(-0.149693, 0.605762),
	vec2(0.034211, 0.979980),
	vec2(0.503098, -0.308878),
	vec2(-0.016205, -0.872921),
	vec2(0.385784, -0.393902),
	vec2(-0.146886, -0.859249),
	vec2(0.643361, 0.164098),
	vec2(0.634388, -0.049471),
	vec2(-0.688894, 0.007843),
	vec2(0.464034, -0.188818),
	vec2(-0.440840, 0.137486),
	vec2(0.364483, 0.511704),
	vec2(0.034028, 0.325968),
	vec2(0.099094, -0.308023),
	vec2(0.693960, -0.366253),
	vec2(0.678884, -0.204688),
	vec2(0.001801, 0.780328),
	vec2(0.145177, -0.898984),
	vec2(0.062655, -0.611866),
	vec2(0.315226, -0.604297),
	vec2(-0.780145, 0.486251),
	vec2(-0.371868, 0.882138),
	vec2(0.200476, 0.494430),
	vec2(-0.494552, -0.711051),
	vec2(0.612476, 0.705252),
	vec2(-0.578845, -0.768792),
	vec2(-0.772454, -0.090976),
	vec2(0.504440, 0.372295),
	vec2(0.155736, 0.065157),
	vec2(0.391522, 0.849605),
	vec2(-0.620106, -0.328104),
	vec2(0.789239, -0.419965),
	vec2(-0.545396, 0.538133),
	vec2(-0.178564, -0.596057)
);

#ifdef COLORED_SHADOWS
vec4 getShadowColor(
	sampler2D shadowsampler, vec2 smTexCoord, float realDistance,
	float normal_length, float perspective_factor, float shadowfar, float textureresolution)
{
	float radius = getPenumbraRadius(
		shadowsampler, smTexCoord, realDistance,
		normal_length, perspective_factor, shadowfar, textureresolution);
	if (radius < 0.1) {
		return getHardShadowColor(shadowsampler, smTexCoord.xy, realDistance);
	}

	vec2 clampedpos;
	vec4 visibility = vec4(0.0);
	float scale_factor = radius / textureresolution;

	int samples = (1 + 1 * int(SOFTSHADOWRADIUS > 1.0)) * PCFSAMPLES;
	samples = int(clamp(pow(4.0 * radius + 1.0, 2.0), 1.0, float(samples)));
	int init_offset = int(floor(mod(((smTexCoord.x * 34.0) + 1.0) * smTexCoord.y, 64.0-samples)));
	int end_offset = int(samples) + init_offset;

	for (int x = init_offset; x < end_offset; x++) {
		clampedpos = poissonDisk[x] * scale_factor + smTexCoord.xy;
		visibility += getHardShadowColor(shadowsampler, clampedpos.xy, realDistance);
	}

	return visibility / samples;
}
#else
float getShadow(
	sampler2D shadowsampler, vec2 smTexCoord, float realDistance,
	float normal_length, float perspective_factor, float shadowfar, float textureresolution)
{
	float radius = getPenumbraRadius(
		shadowsampler, smTexCoord, realDistance,
		normal_length, perspective_factor, shadowfar, textureresolution);
	if (radius < 0.1) {
		return getHardShadow(shadowsampler, smTexCoord.xy, realDistance);
	}

	vec2 clampedpos;
	float visibility = 0.0;
	float scale_factor = radius / textureresolution;

	int samples = (1 + 1 * int(SOFTSHADOWRADIUS > 1.0)) * PCFSAMPLES;
	samples = int(clamp(pow(4.0 * radius + 1.0, 2.0), 1.0, float(samples)));
	int init_offset = int(floor(mod(((smTexCoord.x * 34.0) + 1.0) * smTexCoord.y, 64.0-samples)));
	int end_offset = int(samples) + init_offset;

	for (int x = init_offset; x < end_offset; x++) {
		clampedpos = poissonDisk[x] * scale_factor + smTexCoord.xy;
		visibility += getHardShadow(shadowsampler, clampedpos.xy, realDistance);
	}

	return visibility / samples;
}
#endif

#else // POISSON_FILTER disabled

#ifdef COLORED_SHADOWS
vec4 getShadowColor(
	sampler2D shadowsampler, vec2 smTexCoord, float realDistance,
	float normal_length, float perspective_factor, float shadowfar, float textureresolution)
{
	float radius = getPenumbraRadius(
		shadowsampler, smTexCoord, realDistance,
		normal_length, perspective_factor, shadowfar, textureresolution);
	if (radius < 0.1) {
		return getHardShadowColor(shadowsampler, smTexCoord.xy, realDistance);
	}

	vec2 clampedpos;
	vec4 visibility = vec4(0.0);
	float x, y;
	float bound = (1 + 0.5 * int(SOFTSHADOWRADIUS > 1.0)) * PCFBOUND;
	bound = clamp(0.5 * (4.0 * radius - 1.0), 0.5, bound);
	float scale_factor = radius / bound / textureresolution;
	float n = 0.0;

	for (y = -bound; y <= bound; y += 1.0)
	for (x = -bound; x <= bound; x += 1.0) {
		clampedpos = vec2(x,y) * scale_factor + smTexCoord.xy;
		visibility += getHardShadowColor(shadowsampler, clampedpos.xy, realDistance);
		n += 1.0;
	}

	return visibility / max(n, 1.0);
}
#else
float getShadow(
	sampler2D shadowsampler, vec2 smTexCoord, float realDistance,
	float normal_length, float perspective_factor, float shadowfar, float textureresolution)
{
	float radius = getPenumbraRadius(
		shadowsampler, smTexCoord, realDistance,
		normal_length, perspective_factor, shadowfar, textureresolution);
	if (radius < 0.1) {
		return getHardShadow(shadowsampler, smTexCoord.xy, realDistance);
	}

	vec2 clampedpos;
	float visibility = 0.0;
	float x, y;
	float bound = (1 + 0.5 * int(SOFTSHADOWRADIUS > 1.0)) * PCFBOUND;
	bound = clamp(0.5 * (4.0 * radius - 1.0), 0.5, bound);
	float scale_factor = radius / bound / textureresolution;
	float n = 0.0;

	for (y = -bound; y <= bound; y += 1.0)
	for (x = -bound; x <= bound; x += 1.0) {
		clampedpos = vec2(x,y) * scale_factor + smTexCoord.xy;
		visibility += getHardShadow(shadowsampler, clampedpos.xy, realDistance);
		n += 1.0;
	}

	return visibility / max(n, 1.0);
}
#endif

#endif
