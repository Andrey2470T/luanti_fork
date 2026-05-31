uniform float mGamma = 2.2;

vec4 srgb_to_linear(vec4 color, float gamma)
{
	return pow(color, vec4(gamma));
}

vec4 linear_to_srgb(vec4 color, float gamma)
{
	return pow(color, vec4(1.0/gamma));
}

vec3 ACESFilm(vec3 x) {
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

const float e = 2.718281828459;
const float BS = 10.0;

float smoothCurve(float x)
{
	return x * x * (3.0 - 2.0 * x);
}

float triangleWave(float x)
{
	return abs(fract(x + 0.5) * 2.0 - 1.0);
}

float smoothTriangleWave(float x)
{
	return smoothCurve(triangleWave(x)) * 2.0 - 1.0;
}

// assuming near is always 1.0
// 'depth' should be gl_FragCoord.z (accessible only in fragment shader) or something else
float getLinearDepth(float far, float depth)
{
	return 2.0 * far / (far + 1.0 - (2.0 * depth - 1.0) * (far - 1.0));
}

#if __VERSION__ >= 130
#define mtsmoothstep smoothstep
#else
float mtsmoothstep(in float edge0, in float edge1, in float x)
{
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}
#endif
