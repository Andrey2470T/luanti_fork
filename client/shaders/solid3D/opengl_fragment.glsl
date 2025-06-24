#version 320 core

uniform int uTextureUsage0;
uniform sampler2D uTextureUnit0;

layout (std140) uniform fogParams {
    int enable;
    int type;
    vec4 color;
    float start;
    float end;
    float density;
};

in vec2 vTextureCoord0;
in vec4 vVertexColor;
in float vFogCoord;

float computeFog()
{
	const float LOG2 = 1.442695;
	float FogFactor = 0.0;

	if (uFogType == 0) // Exp
	{
		FogFactor = exp2(-fogParams.density * vFogCoord * LOG2);
	}
	else if (fogParams.type == 1) // Linear
	{
		float Scale = 1.0 / (fogParams.end - fogParams.start);
		FogFactor = (fogParams.end - vFogCoord) * Scale;
	}
	else if (fogParams.type == 2) // Exp2
	{
		FogFactor = exp2(-fogParams.density * fogParams.density * vFogCoord * vFogCoord * LOG2);
	}

	FogFactor = clamp(FogFactor, 0.0, 1.0);

	return FogFactor;
}

void main()
{
	vec4 Color = vVertexColor;

	if (bool(uTextureUsage0))
		Color *= texture2D(uTextureUnit0, vTextureCoord0);

	if (bool(fogParams.enable))
	{
		float FogFactor = computeFog();
		vec4 FogColor = fogParams.color;
		FogColor.a = 1.0;
		Color = mix(FogColor, Color, FogFactor);
	}

	gl_FragColor = Color;
}
