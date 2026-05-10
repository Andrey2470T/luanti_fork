uniform lowp vec4 materialColor;

out lowp vec4 varColor;

out highp vec3 eyeVec;

void main(void)
{
	gl_Position = mWorldViewProj * vec4(inPosition, 1.0);

	vec4 color = inColor;

	float avg_light = (materialColor.r + materialColor.g) / 2.0;
	//color *= avg_light;
	varColor = color;

	eyeVec = -(mWorldView * vec4(inPosition, 1.0)).xyz;
}
