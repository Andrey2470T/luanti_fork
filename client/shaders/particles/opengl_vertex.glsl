#extension GL_EXT_gpu_shader4 : enable // for bitwise operators
#extension GL_ARB_shader_bit_encoding : enable // for floatBitsToInt and intBitsToFloat

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoords; // relatively to the current tile size

#include <matrices>
#include <data_unpack>
#include <final_light_color>

uniform float mDayNightRatio;

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

	vec4 transformedPos = transform * vec4(pos, 1.0);

	int texSize = textureSize(dataTex, 0).x;
	sampleCoords = shiftCoords(sampleCoords, texSize, 16);
	vec2 coloruv = vec2(sampleCoords) / float(texSize);

	vec4 lightColor = texture2D(dataTex, coloruv);
	float nightRatio = 1.0 - lightColor.a;
	vColor = finalLightColor(mDayNightRatio, lightColor, nightRatio);

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
