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

	if (FogParams.enable) {
		float fogFactor = computeFog(vViewPos);
		vec4 fogColor = vec4(FogParams.color_r, FogParams.color_g, FogParams.color_b, FogParams.color_a);
		Color = mix(fogColor, Color, fogFactor);
	}

	outColor = Color;
}
