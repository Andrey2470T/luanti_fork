uniform sampler2D ColorMapSampler;
in vec4 tPos;

CENTROID_ in mediump vec2 varTexCoord;

void main()
{
	vec4 col = texture2D(ColorMapSampler, varTexCoord);

	if (col.a < 0.70)
		discard;

	float depth = 0.5 + tPos.z * 0.5;
	outputColor(vec4(depth, 0.0, 0.0, 1.0));
}
