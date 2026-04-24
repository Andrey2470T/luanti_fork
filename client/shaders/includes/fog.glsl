uniform lowp vec4 fogColor;
uniform float fogDistance;
uniform float fogShadingParameter;

vec4 mixColorWithFog(vec4 color, vec3 eyeVec)
{
	// Due to a bug in some (older ?) graphics stacks (possibly in the glsl compiler ?),
	// the fog will only be rendered correctly if the last operation before the
	// clamp() is an addition. Else, the clamp() seems to be ignored.
	// E.g. the following won't work:
	//      float clarity = clamp(fogShadingParameter
	//		* (fogDistance - length(eyeVec)) / fogDistance), 0.0, 1.0);
	// As additions usually come for free following a multiplication, the new formula
	// should be more efficient as well.
	// Note: clarity = (1 - fogginess)
	float clarity = clamp(fogShadingParameter
		- fogShadingParameter * length(eyeVec) / fogDistance, 0.0, 1.0);
	float fogColorMax = max(max(fogColor.r, fogColor.g), fogColor.b);
	// Prevent zero division.
	if (fogColorMax < 0.0000001) fogColorMax = 1.0;
	// For high clarity (light fog) we tint the fog color.
	// For this to not make the fog color artificially dark we need to normalize using the
	// fog color's brightest value. We then blend our base color with this to make the fog.
	return mix(fogColor * pow(fogColor / fogColorMax, vec4(2.0 * clarity)), color, clarity);
}
