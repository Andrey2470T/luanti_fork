#define baseTexture mTexture0
#define normalTexture mTexture1
#define textureFlags mTexture2

uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D textureFlags;

in mediump vec2 vUV;

out vec4 outColor;

void main(void)
{
	vec4 left = texture2D(baseTexture, vUV).rgba;
	vec4 right = texture2D(normalTexture, vUV).rgba;
	vec4 mask = texture2D(textureFlags, vUV).rgba;
	vec4 color;
	if (mask.r > 0.5)
		color = right;
	else
		color = left;
	outColor = color;
}
