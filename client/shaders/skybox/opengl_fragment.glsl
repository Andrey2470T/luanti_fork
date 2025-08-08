in highp vec3 vEyeVec;

in lowp vec4 vColor;
in highp vec3 vEyeVec;

#include <fog>

out vec4 outColor;

void main(void)
{
	vec4 col = vColor;

    if (bool(mFogParams.enable))
	{
		float FogFactor = computeFog(vEyeVec);
		vec4 FogColor = mFogParams.color;
		FogColor.a = 1.0;
		col = mix(FogColor, col, FogFactor);
	}

	outColor = col;
}
