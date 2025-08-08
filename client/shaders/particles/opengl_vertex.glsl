layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;

#include <matrices>
#include <data_unpack>

layout (binding = 1) uniform sampler2D mDataTex;
uniform int mParticleCount;
uniform int mSampleDim;
uniform int mDataTexDim;

out lowp vec4 vColor;
out highp vec3 vEyeVec;
out ivec2 vTileCoords;
out ivec2 vTileSize;
out vec2 vUV;

void main(void)
{
	ivec2 sampleCoords = getSampleCoords(mDataTex, mParticleCount, mSampleDim, mDataTexDim, gl_InstanceID);

	mat4 transform = unpackFloatMat4x4(mDataTex, sampleCoords);

	vec4 transformedPos = transform * pos;

	int texSize = textureSize(mDataTex, 0).x;
	sampleCoords = shiftCoords(sampleCoords, texSize, 16);
	vec2 coloruv = sampleCoords / texSize;
	vColor = texture2D(dataTex, coloruv);

	sampleCoords = shiftCoords(sampleCoords, texSize);
	vTileCoords = unpackIntVec2(mDataTex, sampleCoords);

	sampleCoords = shiftCoords(sampleCoords, texSize, 2);
	vTileSize = unpackIntVec2(mDataTex, sampleCoords);

	gl_Position = mMatrices.worldViewProj * transformedPos;

	vUV = uv;

	vEyeVec = -(mMatrices.worldView * transformedPos).xyz;
}
