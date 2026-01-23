precision mediump float;

#include <blending>
#include <fog>

uniform int mTextureUsage0;
uniform sampler2D mTexture0;
uniform sampler2D mFramebuffer;
uniform int mBlendMode;

in vec2 vTexCoord;
in vec4 vVertexColor;
in vec3 vViewPos;

out vec4 outColor;

void main()
{
	vec4 Color = vVertexColor;

	if (bool(mTextureUsage0)) {
		Color *= texelFetch(mTexture0, ivec2(vTexCoord.x, vTexCoord.y), 0);
	}
	
	if (FogParams.enable) {
		float fogFactor = computeFog(vViewPos);
		vec4 fogColor = vec4(FogParams.color_r, FogParams.color_g, FogParams.color_b, FogParams.color_a);
		Color = mix(fogColor, Color, fogFactor);
	}
	
	ivec2 fbCoords = ivec2(gl_FragCoord.x, gl_FragCoord.y);
	Color = DoBlend(Color, texelFetch(mFramebuffer, fbCoords, 0), mBlendMode);

	outColor = Color;
}