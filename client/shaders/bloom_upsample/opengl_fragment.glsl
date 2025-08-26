#define current mTexture0
#define previous mTexture1

uniform sampler2D current;
uniform sampler2D previous;
uniform mediump float mBloomRadius;

#ifdef GL_ES
in mediump vec2 vUV;
#else
centroid in vec2 vUV;
#endif

out vec4 outColor;

void main(void)
{
    vec2 texelSize = 1.0f / textureSize(rendered, 0);
	vec2 offset = mBloomRadius * texelSize;

	vec3 a = texture2D(previous, vUV + vec2(-1., -1.) * offset).rgb;
	vec3 b = texture2D(previous, vUV + vec2(0., -1.) * offset).rgb;
	vec3 c = texture2D(previous, vUV + vec2(1., -1.) * offset).rgb;
	vec3 d = texture2D(previous, vUV + vec2(-1., 0.) * offset).rgb;
	vec3 e = texture2D(previous, vUV + vec2(0., 0.) * offset).rgb;
	vec3 f = texture2D(previous, vUV + vec2(1., 0.) * offset).rgb;
	vec3 g = texture2D(previous, vUV + vec2(-1., 1.) * offset).rgb;
	vec3 h = texture2D(previous, vUV + vec2(0., 1.) * offset).rgb;
	vec3 i = texture2D(previous, vUV + vec2(1., 1.) * offset).rgb;

	vec3 base = texture2D(current, vUV).rgb;

	outColor = max(vec4(base +
			(a + c + g + i) * 0.0625 +
			(b + d + f + h) * 0.125 +
			e * 0.25, 1.), 1e-4);
}
