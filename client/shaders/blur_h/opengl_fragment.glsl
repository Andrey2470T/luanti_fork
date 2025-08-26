#define rendered mTexture0

uniform sampler2D rendered;
uniform mediump float mBloomRadius;

#ifdef GL_ES
in mediump vec2 vUV;
#else
centroid in vec2 vUV;
#endif

// smoothstep - squared
float smstsq(float f)
{
	f = f * f * (3. - 2. * f);
	return f;
}

out vec4 outColor;

void main(void)
{
	// kernel distance and linear size
	mediump float n = 2. * mBloomRadius + 1.;

    vec2 texelSize = 1.0f / textureSize(rendered, 0);
	vec2 uv = vUV - vec2(mBloomRadius * texelSize.x, 0.);
	vec4 color = vec4(0.);
	mediump float sum = 0.;
	for (mediump float i = 0.; i < n; i++) {
		mediump float weight = smstsq(1. - (abs(i / mBloomRadius - 1.)));
		color.rgb += texture2D(rendered, uv).rgb * weight;
		sum += weight;
		uv += vec2(texelSize.x, 0.);
	}
	color /= sum;
	outColor = vec4(color.rgb, 1.0); // force full alpha to avoid holes in the image.
}
