CENTROID_ out mediump vec2 varTexCoord;

void main()
{
	vec4 pos = vec4(inPosition, 1.0);
	varTexCoord = (pos * 0.5 + 0.5).st;
	gl_Position = pos;
}
