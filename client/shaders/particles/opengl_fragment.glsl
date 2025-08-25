#define baseTexture mTexture0
#define framebuffer mTexture2
uniform sampler2D baseTexture;
uniform sampler2D framebuffer;

#include <blending>

in lowp vec4 vColor;
in highp vec3 vEyeVec;
in ivec2 vTileCoords;
in ivec2 vTileSize;
in vec2 vUV;
in int vBlendMode;

#include <fog>

out vec4 outColor;

void main(void)
{
	// Calculate the pixel coords of the tile
	int atlas_x = vTileCoords.x + vUV.x * vTileSize.x;
	int atlas_y = vTileCoords.y + vUV.y * vTileCoords.y;

	vec4 base = texelFetch(baseTexture, vec2(atlas_x, atlas_y), 0).rgba;

	vec4 col = vec4(base.rgb * vColor.rgb, 1.0);

    if (bool(mFogParams.enable))
	{
		float FogFactor = computeFog(vEyeVec);
		vec4 FogColor = mFogParams.color;
		FogColor.a = 1.0;
		col = mix(FogColor, col, FogFactor);
	}

	col = DoBlend(col, texelFetch(framebuffer, gl_FragCoord.xy, 0), vBlendMode);

	outColor = col;
}
