out mediump vec4 varTexCoord;

void main(void)
{
	varTexCoord = inTexCoord0;
	gl_Position = inPosition;
}
