layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 uv;

out mediump vec2 vUV;

void main(void)
{
	vUV = uv;
	gl_Position = vec4(pos, 0.0, 1.0);
}
