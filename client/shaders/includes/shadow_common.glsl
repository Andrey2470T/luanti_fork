// assuming near is always 1.0
float getLinearDepth(float shadowfar)
{
	return 2.0 * shadowfar / (shadowfar + 1.0 - (2.0 * gl_FragCoord.z - 1.0) * (shadowfar - 1.0));
}

vec3 getLightSpacePosition(vec3 shadow_pos)
{
	return shadow_pos * 0.5 + 0.5;
}

// custom smoothstep implementation because it's not defined in glsl1.2
float mtsmoothstep(in float edge0, in float edge1, in float x)
{
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

#ifdef COLORED_SHADOWS
// c_precision of 128 fits within 7 base-10 digits
const float c_precision = 128.0;
const float c_precisionp1 = c_precision + 1.0;

float packColor(vec3 color)
{
	return floor(color.b * c_precision + 0.5)
		+ floor(color.g * c_precision + 0.5) * c_precisionp1
		+ floor(color.r * c_precision + 0.5) * c_precisionp1 * c_precisionp1;
}

vec3 unpackColor(float value)
{
	vec3 color;
	color.b = mod(value, c_precisionp1) / c_precision;
	color.g = mod(floor(value / c_precisionp1), c_precisionp1) / c_precision;
	color.r = floor(value / (c_precisionp1 * c_precisionp1)) / c_precision;
	return color;
}
#endif
