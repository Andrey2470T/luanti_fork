uniform lowp vec4 materialColor;

out lowp vec4 varColor;

out highp vec3 eyeVec;

void main(void)
{
	gl_Position = mWorldViewProj * vec4(inPosition, 1.0);

	vec4 color = inColor;

	color *= materialColor;
	varColor = color;

	eyeVec = -(mWorldView * vec4(inPosition, 1.0)).xyz;
}
