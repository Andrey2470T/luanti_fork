// Returns the sample pixel coords
ivec2 getSampleCoords(sampler2D dataTex, int sampleCount, int sampleDim, int texDim, int sampleN)
{
	if (sampleN > sampleCount-1)
		return ivec2(0, 0);

	int x = sampleN % texDim * sampleDim;
	int y = (sampleN+1) / texDim * sampleDim;

	return ivec2(x, y);
}

int unpackInt(sampler2D dataTex, ivec2 coords)
{
	vec4 int_v = texelFetch(dataTex, coords, 0);

	int int_n;
	int_n = int_n | (int_v.r << 24);
	int_n = int_n | (int_v.g << 16);
	int_n = int_n | (int_v.b << 8);
	int_n = int_n | int_v.a;

	return int_n;
}

float unpackFloat(sampler2D dataTex, ivec2 coords)
{
	int int_v = unpackInt(dataTex, coords);

	return intBitsToFloat(int_v);
}

ivec2 shiftCoords(ivec2 coords, int width, int n=1)
{
	ivec2 shifted_c;

	if (coords.x+n > width-1) {
		shifted_c.y = coords.y+1;
		shifted_c.x = n - (width - coords.x+1) - 1;
	}
	else {
		shifted_c.x = coords.x+n;
	}

	return shifted_c;
}

ivec2 unpackIntVec2(sampler2D dataTex, ivec2 coords)
{
	int texSize = textureSize(dataTex, 0).x;
	int x = unpackInt(dataTex, coords);
	int y = unpackInt(dataTex, shiftCoords(coords, texSize));

	return ivec2(x, y);
}

ivec3 unpackIntVec3(sampler2D dataTex, ivec2 coords)
{
	int texSize = textureSize(dataTex, 0).x;
	int x = unpackInt(dataTex, coords);
	coords = shiftCoords(coords, texSize);
	int y = unpackInt(dataTex, coords);
	int z = unpackInt(dataTex, shiftCoords(coords, texSize));

	return ivec3(x, y, z);
}

ivec4 unpackIntVec4(sampler2D dataTex, ivec2 coords)
{
	int texSize = textureSize(dataTex, 0).x;
	int x = unpackInt(dataTex, coords);
	coords = shiftCoords(coords, texSize);
	int y = unpackInt(dataTex, coords);
	coords = shiftCoords(coords, texSize);
	int z = unpackInt(dataTex, coords);
	int w = unpackInt(dataTex, shiftCoords(coords, texSize));

	return ivec4(x, y, z, w);
}

vec2 unpackFloatVec2(sampler2D dataTex, ivec2 coords)
{
	int texSize = textureSize(dataTex, 0).x;
	float x = unpackFloat(dataTex, coords);
	float y = unpackFloat(dataTex, shiftCoords(coords, texSize));

	return vec2(x, y);
}

vec3 unpackFloatVec3(sampler2D dataTex, ivec2 coords)
{
	int texSize = textureSize(dataTex, 0).x;
	float x = unpackFloat(dataTex, coords);
	coords = shiftCoords(coords, texSize);
	float y = unpackFloat(dataTex, coords);
	float z = unpackFloat(dataTex, shiftCoords(coords, texSize));

	return vec3(x, y, z);
}

vec4 unpackFloatVec4(sampler2D dataTex, ivec2 coords)
{
	int texSize = textureSize(dataTex, 0).x;
	float x = unpackFloat(dataTex, coords);
	coords = shiftCoords(coords, texSize);
	float y = unpackFloat(dataTex, coords);
	coords = shiftCoords(coords, texSize);
	float z = unpackFloat(dataTex, coords);
	float w = unpackFloat(dataTex, shiftCoords(coords, texSize));

	return vec4(x, y, z, w);
}

mat4 unpackFloatMat4x4(sampler2D dataTex, ivec2 coords)
{
	int texSize = textureSize(dataTex, 0).x;
	vec4 firstRow = unpackFloatVec4(dataTex, coords);
	coords = shiftCoords(coords, texSize, 4);
	vec4 secondRow = unpackFloatVec4(dataTex, coords);
	coords = shiftCoords(coords, texSize, 4);
	vec4 thirdRow = unpackFloatVec4(dataTex, coords);
	coords = shiftCoords(coords, texSize, 4);
	vec4 fourthRow = unpackFloatVec4(dataTex, coords);

	mat4 floatM;
	// matrices in glsl are column-major!
	floatM[0] = vec4(firstRow.x, secondRow.x, thirdRow.x, fourthRow.x);
	floatM[1] = vec4(firstRow.y, secondRow.y, thirdRow.y, fourthRow.y);
	floatM[2] = vec4(firstRow.z, secondRow.z, thirdRow.z, fourthRow.z);
	floatM[3] = vec4(firstRow.w, secondRow.w, thirdRow.w, fourthRow.w);

	return floatM;
}
