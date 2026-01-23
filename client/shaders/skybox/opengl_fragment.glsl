in lowp vec4 vColor;
in vec3 vEyeVec;

#include <fog>

out vec4 outColor;

void main(void)
{
	vec4 col = vColor;

	if (FogParams.enable) {
		float fogFactor = computeFog(vEyeVec);
		vec4 fogColor = vec4(FogParams.color_r, FogParams.color_g, FogParams.color_b, FogParams.color_a);
		col = mix(fogColor, col, fogFactor);
	}

	outColor = col;
}
