layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 uv;

void main()
{
	vec4 uv = vec4(pos, 1.0) * 0.5 + 0.5;
	gl_TexCoord[0] = uv;
	gl_TexCoord[1] = uv;
	gl_TexCoord[2] = uv;
	gl_Position = vec4(pos, 1.0);
}
