precision mediump float;

uniform int mTextureUsage0;
uniform sampler2D mTexture0;

in vec2 vUV0;
in vec4 vVertexColor;

out vec4 outColor;

void main()
{
	vec4 Color = vVertexColor;

	if (bool(mTextureUsage0))
		Color *= texture2D(mTexture0, vUV0);

	outColor = Color;
}
