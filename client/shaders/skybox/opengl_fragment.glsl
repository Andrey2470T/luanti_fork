in lowp vec4 vColor;
in highp vec3 vEyeVec;

#include <fog>

out vec4 outColor;

void main(void)
{
	vec4 col = vColor;

    if (bool(FogParams.enable))
	{
		float FogFactor = computeFog(vEyeVec);
		vec4 FogColor = FogParams.color;
		FogColor.a = 1.0;
		col = mix(col, FogColor, FogFactor);
	}

	outColor = col;
}
