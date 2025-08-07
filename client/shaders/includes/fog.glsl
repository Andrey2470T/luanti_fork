layout (std140) uniform mFogParams {
    int enable;
    int type;
    vec4 color;
    float start;
    float end;
    float density;
};

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
