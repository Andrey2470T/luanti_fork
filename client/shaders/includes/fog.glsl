layout (std140) uniform mFogParams {
    int enable;
    int type;
    vec4 color;
    float start;
    float end;
    float density;
} FogParams;

float computeFog(vec3 viewPos)
{
	const float LOG2 = 1.442695;
	float FogFactor = 0.0;

	float FogCoord = length(viewPos);

	if (FogParams.type == 0) // Exp
	{
		FogFactor = exp2(-FogParams.density * FogCoord * LOG2);
	}
	else if (FogParams.type == 1) // Linear
	{
		float Scale = 1.0 / (FogParams.end - FogParams.start);
		FogFactor = (FogParams.end - FogCoord) * Scale;
	}
	else if (FogParams.type == 2) // Exp2
	{
		FogFactor = exp2(-FogParams.density * FogParams.density * FogCoord * FogCoord * LOG2);
	}

	FogFactor = clamp(FogFactor, 0.0, 1.0);

	return FogFactor;
}
