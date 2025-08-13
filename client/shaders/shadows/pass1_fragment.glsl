uniform sampler2D ColorMapSampler;
in vec4 tPos;

// the depth texture uses the GL_R32F format
out float outDepth;

void main()
{
	vec4 col = texture2D(ColorMapSampler, gl_TexCoord[0].st);

	if (col.a < 0.70)
		discard;

	float depth = 0.5 + tPos.z * 0.5;
	outDepth = depth;
}
