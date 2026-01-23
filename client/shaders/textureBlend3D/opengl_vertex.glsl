layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;

#include <matrices>

uniform float mThickness;

out vec2 vTexCoord;
out vec4 vVertexColor;
out vec3 vViewPos;

void main()
{
	gl_Position = Matrices.worldViewProj * vec4(pos, 1.0);
	gl_PointSize = mThickness;

	//vec4 TextureCoord0 = vec4(uv.x, uv.y, 0.0, 0.0);
	vTexCoord = uv;//vec4(Matrices.texture0 * TextureCoord0).xy;

	vVertexColor = color;
	vViewPos = (Matrices.worldView * vec4(pos, 1.0)).xyz;
}