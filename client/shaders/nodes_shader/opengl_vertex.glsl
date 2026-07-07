#include<lighting>
#include<noise>
#include<vertex_animations>

#define CRACK_FRAME_SIZE 16

uniform vec3 lightDir;
uniform float timeOfDay;

out vec3 vNormal;
out vec3 vPosition;
// World position in the visible world (i.e. relative to the cameraOffset.)
// This can be used for many shader effects without loss of precision.
// If the absolute position is required it can be calculated with
// cameraOffset + worldPosition (for large coordinates the limits of float
// precision must be considered).
out vec3 worldPosition;
// The centroid keyword ensures that after interpolation the texture coordinates
// lie within the same bounds when MSAA is en- and disabled.
// This fixes the stripes problem with nearest-neighbor textures and MSAA.
CENTROID_ out lowp float emissionLight;
CENTROID_ out lowp vec3 varColor;
CENTROID_ out lowp vec3 dayLight;
CENTROID_ out mediump vec2 varTexCoord;
CENTROID_ out float nightRatio;

#ifdef ENABLE_DYNAMIC_SHADOWS
	out float adj_shadow_strength;
	out float cosLight;
	out float f_normal_length;
	out vec3 shadow_position;
	out float perspective_factor;
#endif

out highp vec3 eyeVec;
out vec3 hwColor;
out vec2 crackTexCoord;
out float hasCrack;

void main(void)
{
	varTexCoord = (mTexture * vec4(inTexCoord0.xy, 1.0, 1.0)).st;

	vec4 pos = vec4(inPosition, 1.0);
	worldPosition = (mWorld * pos).xyz;

	uint packedR = floatBitsToUint(inAux.x);
	uint packedG = floatBitsToUint(inAux.y);
	uint packedB = floatBitsToUint(inAux.z);

	// Vertex animations depending on the material type
#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES)
	animateLeavesVertex(worldPosition, pos);
#elif (MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS)
	vec2 tileCoords = vec2(packedG >> 16, packedG & 0xffffu);
	// Extract the tile dimensions (width and height)
	vec2 tileSize = vec2(packedB >> 24, packedB >> 16 & 0xffu);
	animatePlantVertex(worldPosition, pos, tileCoords / tileSize);
#elif MATERIAL_WAVING_LIQUID && ENABLE_WAVING_WATER
	animateWaterVertex(worldPosition, pos);
#endif

	gl_Position = mWorldViewProj * pos;

	vPosition = gl_Position.xyz;
	eyeVec = -(mWorldView * pos).xyz;
	vNormal = inNormal;

	// Calculate the light color
	vec4 color = inColor;
	float skyLight = color.r;
	float blockLight = color.g;
	float ao = color.b;
	float sum = float(max(skyLight + blockLight, 0));
	nightRatio = 1.0 - (skyLight / sum);
	dayLight = getSkyColor(timeOfDay);
	varColor = calculateLighting(timeOfDay, skyLight, blockLight, ao);

	// Calculate the emission factor for the bloom mask
	vec3 lightColor = (max(dayLight * skyLight, blockColor * (blockLight-color.a)) + ambientColor) * ao;
	emissionLight = color.a * (1.0 - luminance(lightColor));

	// inAux structure:
	// Channel | Bits count | Value
	// red     |     8      | hwColor.r
	// red     |     8      | hwColor.g
	// red     |     8      | hwColor.b
	// red     |     1      | hasCrack
	// red     |     7      |   -
	// green   |    16      | crackTexCoord.x
	// green   |    16      | crackTexCoord.y
	// blue    |     8      | width
	// blue    |     8      | height
	// blue    |    16      |   -

	// Extract the hw color and crack flag
	hwColor.r = float(packedR >> 24) / 255.0;
	hwColor.g = float(packedR >> 16 & 0xffu) / 255.0;
	hwColor.b = float(packedR >> 8 & 0xffu) / 255.0;

	hasCrack = packedR & 0x1u;

	// Extract the crack texcoords in UV space
	crackTexCoord.x = float(packedG >> 16) / CRACK_FRAME_SIZE;
	crackTexCoord.y = float(packedG & 0xffffu) / CRACK_FRAME_SIZE;

#ifdef ENABLE_DYNAMIC_SHADOWS
#if MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS
	// The shadow shaders don't apply waving when creating the shadow-map.
	// We are using the not waved inPosition to avoid ugly self-shadowing.
	vec4 shadow_pos = vec4(inPosition, 1.0);
#else
	vec4 shadow_pos = pos;
#endif
	vertexStage(
	    shadow_pos, vNormal, lightDir, timeOfDay, f_normal_length, cosLight,
		shadow_position, perspective_factor, adj_shadow_strength
	);
#endif
}
