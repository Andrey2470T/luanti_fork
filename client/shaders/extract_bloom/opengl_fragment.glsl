#define bloomMask texture0

struct ExposureParams {
	float compensationFactor;
};

uniform mediump float bloomStrength;
uniform ExposureParams exposureParams;

CENTROID_ in mediump vec2 varTexCoord;

#ifdef ENABLE_AUTO_EXPOSURE
in float exposure; // linear exposure factor, see vertex shader
#endif

void main(void)
{
	vec2 uv = varTexCoord.st;
	vec3 color = texture2D(bloomMask, uv).rgb;
	// translate to linear colorspace (approximate)
#ifdef GL_ES
	// clamp color to [0,1] range in lieu of centroids
	color = srgb_to_linear(clamp(color, 0.0, 1.0));
#else
	color = srgb_to_linear(color);
#endif

	color *= exposureParams.compensationFactor * bloomStrength;

#ifdef ENABLE_AUTO_EXPOSURE
	color *= exposure;
#endif

	outputColor(vec4(color, 1.0)); // force full alpha to avoid holes in the image.
}
