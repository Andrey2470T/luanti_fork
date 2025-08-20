uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D textureFlags;

#define leftImage baseTexture
#define rightImage normalTexture
#define maskImage textureFlags

in mediump vec2 vUV;

out vec4 outColor;

void main(void)
{
	vec4 left = texture2D(leftImage, vUV).rgba;
	vec4 right = texture2D(rightImage, vUV).rgba;
	vec4 mask = texture2D(maskImage, vUV).rgba;
	vec4 color;
	if (mask.r > 0.5)
		color = right;
	else
		color = left;
	outColor = color;
}
