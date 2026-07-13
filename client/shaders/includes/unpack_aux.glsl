// inAux structure:
	// Channel | Bits count | Value
	// red     |     8      | hwColor.r
	// red     |     8      | hwColor.g
	// red     |     8      | hwColor.b
	// red     |     7      |   -
	// red     |     1      | hasCrack
	// green   |    16      | crackTexCoord.x
	// green   |    16      | crackTexCoord.y
	// blue    |     8      | width
	// blue    |     8      | height
	// blue    |     1      |         -
	// blue    |     5      | blockLightColor.r
	// blue    |     5      | blockLightColor.g
	// blue    |     5      | blockLightColor.b

void unpackHWColor(in vec3 auxAttr, out vec3 color)
{
	uint packedR = floatBitsToUint(auxAttr.x);
	color.r = float(packedR >> 24) / 255.0;
	color.g = float(packedR >> 16 & 0xffu) / 255.0;
	color.b = float(packedR >> 8 & 0xffu) / 255.0;
}

void unpackCrackFlag(in vec3 auxAttr, out float flag)
{
	uint packedR = floatBitsToUint(auxAttr.x);
	flag = packedR & 0x1u;
}

void unpackAuxRed(in vec3 auxAttr, out vec3 color, out float flag)
{
	unpackHWColor(auxAttr, color);
	unpackCrackFlag(auxAttr, flag);
}

void unpackCrackUV(in vec3 auxAttr, out vec2 uv)
{
	uint packedG = floatBitsToUint(inAux.y);
	uv.x = float(packedG >> 16) / CRACK_FRAME_SIZE;
	uv.y = float(packedG & 0xffffu) / CRACK_FRAME_SIZE;
}

vec2 unpackTileUV(in vec3 auxAttr)
{
	uint packedG = floatBitsToUint(inAux.y);
	uint packedB = floatBitsToUint(inAux.z);

	vec2 tileCoords = vec2(packedG >> 16, packedG & 0xffffu);
	vec2 tileSize = vec2(packedB >> 24, packedB >> 16 & 0xffu);

	return tileCoords / tileSize;
}

vec3 unpackBlockLightColor(in vec3 auxAttr)
{
	uint packedB = floatBitsToUint(inAux.z);

	vec3 color;
	color.r = packedB >> 10 & 0x1fu;
	color.g = packedB >> 5 & 0x1fu;
	color.b = packedB & 0x1fu;

	return color;
}
