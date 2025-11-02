layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoords; // relatively to the current tile size

#include <matrices>
#include <data_unpack>

#define dataTex mTexture1
uniform sampler2D dataTex;
uniform int mSampleCount;
uniform int mSampleDim;
uniform int mDataTexDim;

out lowp vec4 vColor;
out highp vec3 vEyeVec;
out ivec2 vTileCoords;
out ivec2 vTileSize;
out vec2 vTexCoord;
out int vBlendMode;

void main(void)
{
	ivec2 sampleCoords = getSampleCoords(dataTex, mSampleCount, mSampleDim, mDataTexDim, gl_InstanceID);

	mat4 transform = unpackFloatMat4x4(dataTex, sampleCoords);

	vec4 transformedPos = transform * pos;

	int texSize = textureSize(dataTex, 0).x;
	sampleCoords = shiftCoords(sampleCoords, texSize, 16);
	vec2 coloruv = sampleCoords / texSize;
	vColor = texture2D(dataTex, coloruv);

	sampleCoords = shiftCoords(sampleCoords, texSize);
	vTileCoords = unpackIntVec2(dataTex, sampleCoords);

	sampleCoords = shiftCoords(sampleCoords, texSize, 2);
	vTileSize = unpackIntVec2(dataTex, sampleCoords);

	sampleCoords = shiftCoords(sampleCoords, texSize, 1);
	vBlendMode = unpackInt(dataTex, sampleCoords);

	gl_Position = Matrices.worldViewProj * transformedPos;

	vTexCoord = texCoords;

	vEyeVec = -(Matrices.worldView * transformedPos).xyz;
}
