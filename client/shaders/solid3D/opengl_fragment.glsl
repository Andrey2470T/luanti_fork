precision mediump float;

uniform int mTextureUsage0;
uniform sampler2D mTexture0;

#include <fog>

in vec2 vUV0;
in vec4 vVertexColor;
in float vFogCoord;

out vec4 outColor;

void main()
{
	vec4 Color = vVertexColor;

	if (bool(mTextureUsage0))
		Color *= texture2D(mTexture0, vUV0);

	if (bool(mFogParams.enable))
	{
		float FogFactor = computeFog();
		vec4 FogColor = mFogParams.color;
		FogColor.a = 1.0;
		Color = mix(FogColor, Color, FogFactor);
	}

	outColor = Color;
}
