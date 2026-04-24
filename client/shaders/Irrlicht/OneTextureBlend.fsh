#version 150

precision mediump float;

uniform int uTextureUsage0;
uniform sampler2D uTextureUnit0;
uniform int uBlendType;
uniform int uFogEnable;
uniform int uFogType;
uniform vec4 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform float uFogDensity;

in vec2 vTextureCoord0;
in vec4 vVertexColor;
in float vFogCoord;

float computeFog()
{
	const float LOG2 = 1.442695;
	float FogFactor = 0.0;

	if (uFogType == 0) // Exp
	{
		FogFactor = exp2(-uFogDensity * vFogCoord * LOG2);
	}
	else if (uFogType == 1) // Linear
	{
		float Scale = 1.0 / (uFogEnd - uFogStart);
		FogFactor = (uFogEnd - vFogCoord) * Scale;
	}
	else if (uFogType == 2) // Exp2
	{
		FogFactor = exp2(-uFogDensity * uFogDensity * vFogCoord * vFogCoord * LOG2);
	}

	FogFactor = clamp(FogFactor, 0.0, 1.0);

	return FogFactor;
}

out vec4 outColor0;

void main()
{
	vec4 Color0 = vVertexColor;
	vec4 Color1 = vec4(1.0, 1.0, 1.0, 1.0);

	if (bool(uTextureUsage0))
		Color1 = texture2D(uTextureUnit0, vTextureCoord0);

	vec4 FinalColor = Color0 * Color1;

	if (uBlendType == 1)
	{
		FinalColor.w = Color0.w;
	}
	else if (uBlendType == 2)
	{
		FinalColor.w = Color1.w;
	}

	if (bool(uFogEnable))
	{
		float FogFactor = computeFog();
		vec4 FogColor = uFogColor;
		FogColor.a = 1.0;
		FinalColor = mix(FogColor, FinalColor, FogFactor);
	}

	outColor0 = FinalColor;
}
