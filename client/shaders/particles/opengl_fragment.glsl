#define baseTexture mTexture0
#define framebuffer mTexture2
uniform sampler2D baseTexture;
uniform sampler2D framebuffer;

#include <blending>

in lowp vec4 vColor;
in highp vec3 vEyeVec;
flat in ivec2 vTileCoords;
flat in ivec2 vTileSize;
in vec2 vTexCoord;
flat in int vBlendMode;

#include <fog>

out vec4 outColor;

void main(void)
{
	// Calculate the pixel coords of the tile
	int atlas_x = int(vTexCoord.x * float(vTileSize.x)) + vTileCoords.x;
	int atlas_y = int(vTexCoord.y * float(vTileSize.y)) + vTileCoords.y;

	vec4 base = texelFetch(baseTexture, ivec2(atlas_x, atlas_y), 0).rgba;

	vec4 col = vec4(base.rgb * vColor.rgb, 1.0);

    if (FogParams.enable) {
		float fogFactor = computeFog(vEyeVec);
		vec4 fogColor = vec4(FogParams.color_r, FogParams.color_g, FogParams.color_b, FogParams.color_a);
		col = mix(fogColor, col, fogFactor);
	}

	ivec2 fbCoords = ivec2(gl_FragCoord.x, gl_FragCoord.y);
	col = DoBlend(col, texelFetch(framebuffer, fbCoords, 0), vBlendMode);

	outColor = col;
}
