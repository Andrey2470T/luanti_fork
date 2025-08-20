#define rendered texture0

struct ExposureParams {
	float compensationFactor;
};

uniform sampler2D rendered;
uniform mediump float bloomStrength;
uniform ExposureParams exposureParams;

#ifdef GL_ES
in mediump vec2 vUV;
#else
centroid in vec2 vUV;
#endif

#ifdef ENABLE_AUTO_EXPOSURE
in float exposure; // linear exposure factor, see vertex shader
#endif

out vec4 outColor;

void main(void)
{
	vec3 color = texture2D(rendered, vUV).rgb;
	// translate to linear colorspace (approximate)
#ifdef GL_ES
	// clamp color to [0,1] range in lieu of centroids
	color = pow(clamp(color, 0.0, 1.0), vec3(2.2));
#else
	color = pow(color, vec3(2.2));
#endif

	color *= exposureParams.compensationFactor * bloomStrength;

#ifdef ENABLE_AUTO_EXPOSURE
	color *= exposure;
#endif

	outColor = vec4(color, 1.0); // force full alpha to avoid holes in the image.
}
