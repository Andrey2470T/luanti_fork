layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 uv;

#ifdef ENABLE_AUTO_EXPOSURE
#define exposureMap texture1

uniform sampler2D exposureMap;

out float exposure;
#endif

#ifdef GL_ES
out mediump vec2 vUV;
#else
centroid out vec2 vUV;
#endif


void main(void)
{
#ifdef ENABLE_AUTO_EXPOSURE
	// value in the texture is on a logarithtmic scale
	exposure = texture2D(exposureMap, vec2(0.5)).r;
	exposure = pow(2., exposure);
#endif

	vUV = uv;
	gl_Position = vec4(pos, 0.0, 1.0);
}
