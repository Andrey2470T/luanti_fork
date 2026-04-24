out lowp vec4 varColor;
out mediump vec2 varTexCoord;

void main(void)
{
	varTexCoord = inTexCoord0.st;
	gl_Position = mWorldViewProj * vec4(inPosition, 1.0);

	varColor = inColor;
}
