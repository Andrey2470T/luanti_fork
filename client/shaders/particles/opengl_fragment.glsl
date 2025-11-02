#define baseTexture mTexture0
#define framebuffer mTexture2
uniform sampler2D baseTexture;
uniform sampler2D framebuffer;

#include <blending>

in lowp vec4 vColor;
in highp vec3 vEyeVec;
in ivec2 vTileCoords;
in ivec2 vTileSize;
in vec2 vTexCoord;
in int vBlendMode;

#include <fog>

out vec4 outColor;

void main(void)
{
	// Calculate the pixel coords of the tile
	int atlas_x = vTexCoord.x + vTileCoords.x;
	int atlas_y = vTexCoord.y + vTileCoords.y;

	vec4 base = texelFetch(baseTexture, vec2(atlas_x, atlas_y), 0).rgba;

	vec4 col = vec4(base.rgb * vColor.rgb, 1.0);

    if (bool(FogParams.enable))
	{
		float FogFactor = computeFog(vEyeVec);
		vec4 FogColor = FogParams.color;
		FogColor.a = 1.0;
		col = mix(FogColor, col, FogFactor);
	}

	ivec2 fbCoords = ivec2(gl_FragCoord.x, gl_FragCoord.y);
	col = DoBlend(col, texelFetch(framebuffer, fbCoords, 0), vBlendMode);

	outColor = col;
}
