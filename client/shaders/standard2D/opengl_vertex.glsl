layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 uv;

uniform float mThickness;
uniform mat4 mProjection;

out vec2 vUV0;
out vec4 vVertexColor;

void main()
{
	gl_Position = mProjection * vec4(pos, 0.0, 1.0);
	gl_PointSize = mThickness;
	vUV0 = uv;
	vVertexColor = color.bgra;
}
