in highp vec3 eyeVec;

in lowp vec4 varColor;

void main(void)
{
	vec4 col = varColor;

	col = mixColorWithFog(col, eyeVec);

	outColor0 = col;
}
