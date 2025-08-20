layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 uv;

uniform mat4 mWorld;

out vec4 vVertexColor;
out vec2 vUV0;

void main(void)
{
	vUV0 = uv;
	gl_Position = mWorld * vec4(pos, 0.0, 1.0);
	vVertexColor = color;
}
