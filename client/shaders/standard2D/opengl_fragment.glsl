precision mediump float;

uniform int mTextureUsage0;
uniform sampler2D mTexture0;

in vec2 vTexCoord;
in vec4 vVertexColor;

out vec4 outColor;

void main()
{
	vec4 Color = vVertexColor;

	if (bool(mTextureUsage0)) {
		vec4 texColor = texelFetch(mTexture0, ivec2(vTexCoord.x, vTexCoord.y), 0);

		Color *= texColor;
		//if (texColor.r == 0 && texColor.g == 0 && texColor.b == 0 && texColor.a == 1)
		//	Color.a = 0;
	}

	outColor = Color;
}
