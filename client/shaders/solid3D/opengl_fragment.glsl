precision mediump float;

uniform int mTextureUsage0;
uniform sampler2D mTexture0;

layout (std140) uniform mFogParams {
    int enable;
    int type;
    vec4 color;
    float start;
    float end;
    float density;
};

in vec2 vUV0;
in vec4 vVertexColor;
in float vFogCoord;

out vec4 outColor;

float computeFog()
{
	const float LOG2 = 1.442695;
	float FogFactor = 0.0;

	if (mFogParams.type == 0) // Exp
	{
		FogFactor = exp2(-mFogParams.density * vFogCoord * LOG2);
	}
	else if (mFogParams.type == 1) // Linear
	{
		float Scale = 1.0 / (mFogParams.end - mFogParams.start);
		FogFactor = (mFogParams.end - vFogCoord) * Scale;
	}
	else if (mFogParams.type == 2) // Exp2
	{
		FogFactor = exp2(-mFogParams.density * mFogParams.density * vFogCoord * vFogCoord * LOG2);
	}

	FogFactor = clamp(FogFactor, 0.0, 1.0);

	return FogFactor;
}

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
