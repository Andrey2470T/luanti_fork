#ifdef GL_ES
out mediump vec2 varTexCoord;
#else
centroid out vec2 varTexCoord;
#endif

void main(void)
{
	varTexCoord.st = inTexCoord0.st;
	gl_Position = vec4(inPosition, 1.0);
}
