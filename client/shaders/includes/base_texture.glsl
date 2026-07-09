#include<blending>

// The texture saving the current single crack frame updated externally
#define crackFrameTexture texture7
uniform sampler2D crackFrameTexture;

vec4 getTextureColor(in vec2 uv, in int pixelUV, in float hasCrack, in vec2 crackTexCoord)
{
	vec4 color;
	if (bool(pixelUV))
		color = texelFetch(baseTexture, ivec2(uv.x, uv.y), 0);
	else
		color = texture(baseTexture, uv);

	if (bool(hasCrack)) {
		vec4 crack = texture(crackFrameTexture, crackTexCoord);
		color = DoBlend(OVERLAY_BLEND_MODE, color, crack);
	}

	return color;
}
