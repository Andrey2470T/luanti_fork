uniform lowp vec4 fogColor;
uniform float fogDistance;
uniform float fogShadingParameter;

vec4 mixColorWithFog(vec4 color, vec3 eyeVec)
{
	float clarity = clamp(fogShadingParameter
		- fogShadingParameter * length(eyeVec) / fogDistance, 0.0, 1.0);
	vec3 mixColor = mix(fogColor.rgb, color.rgb, clarity);

	return vec4(mixColor, color.a);
}
