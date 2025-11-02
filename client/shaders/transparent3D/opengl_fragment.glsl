precision mediump float;

uniform int mAlphaDiscard;
uniform float mAlphaRef;
uniform int mTextureUsage0;
uniform sampler2D mTexture0;

#include <fog>

in vec2 vTexCoord;
in vec4 vVertexColor;
in float vFogCoord;
in vec3 vViewPos;

out vec4 outColor;

void main()
{
	vec4 Color = vVertexColor;

	if (bool(mTextureUsage0)) {
		Color *= texelFetch(mTexture0, ivec2(vTexCoord.x, vTexCoord.y), 0);
		
		if (mAlphaDiscard == 1 && Color.a < mAlphaRef)
	        discard;
	}
	
	if (mAlphaDiscard == 0 && Color.a < mAlphaRef)
        discard;

	if (bool(FogParams.enable))
	{
		float FogFactor = computeFog(vViewPos);
		vec4 FogColor = FogParams.color;
		FogColor.a = 1.0;
		Color = mix(FogColor, Color, FogFactor);
	}

	outColor = Color;
}
