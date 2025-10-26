precision mediump float;

#include <blending>
#include <fog>

uniform int mTextureUsage0;
uniform sampler2D mTexture0;
uniform sampler2D mFramebuffer;
uniform int mBlendMode;

in vec2 vUV0;
in vec4 vVertexColor;
in vec3 vViewPos;

out vec4 outColor;

void main()
{
	vec4 Color = vVertexColor;

	if (bool(mTextureUsage0)) {
		Color *= texture2D(mTexture0, vUV0);
	}
	
	if (bool(FogParams.enable))
	{
		float FogFactor = computeFog(vViewPos);
		vec4 FogColor = FogParams.color;
		FogColor.a = 1.0;
		Color = mix(FogColor, Color, FogFactor);
	}
	
	Color = DoBlend(Color, texture2D(mFramebuffer, vUV0), mBlendMode);

	outColor = Color;
}