void main(void)
{
	gl_Position = mWorldViewProj * vec4(inPosition, 1.0);
}
