precision mediump float;

uniform int mTextureUsage0;
uniform sampler2D mTexture0;

#include <fog>

in vec2 vUV0;
in vec4 vVertexColor;
in float vFogCoord;
in vec3 vViewPos;

out vec4 outColor;

void main()
{
	vec4 Color = vVertexColor;

	if (bool(mTextureUsage0))
		Color *= texture2D(mTexture0, vUV0);

	if (bool(FogParams.enable))
	{
		float FogFactor = computeFog(vViewPos);
		vec4 FogColor = FogParams.color;
		FogColor.a = 1.0;
		Color = mix(FogColor, Color, FogFactor);
	}

	outColor = Color;
}
