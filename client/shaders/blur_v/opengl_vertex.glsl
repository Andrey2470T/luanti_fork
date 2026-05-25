CENTROID_ out mediump vec2 varTexCoord;

void main(void)
{
	varTexCoord.st = inTexCoord0.st;
	gl_Position = vec4(inPosition, 1.0);
}
