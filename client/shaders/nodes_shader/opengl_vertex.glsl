#include<lighting>
#include<noise>
#include<vertex_animations>

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
	out float cosLight;
	out float normalOffsetScale;
	out float adj_shadow_strength;
	out float f_normal_length;
	out vec3 shadow_position;
	out float perspective_factor;
#endif

out highp vec3 eyeVec;
out vec3 hwColor;

void main(void)
{
	varTexCoord = (mTexture * vec4(inTexCoord0.xy, 1.0, 1.0)).st;

	vec4 pos = vec4(inPosition, 1.0);
	worldPosition = (mWorld * pos).xyz;

	// Vertex animations depending on the material type
#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES)
	animateLeavesVertex(worldPosition, pos);
#elif (MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS)
	animatePlantVertex(worldPosition, pos, varTexCoord);
#elif MATERIAL_WAVING_LIQUID && ENABLE_WAVING_WATER
	animateWaterVertex(worldPosition, pos);
#endif

	gl_Position = mWorldViewProj * pos;

	vPosition = gl_Position.xyz;
	eyeVec = -(mWorldView * pos).xyz;
	vNormal = inNormal;

	// Calculating the light color
	vec4 color = inColor;
	float skyLight = color.r;
	float blockLight = color.g;
	float ao = color.b;
	float sum = float(max(skyLight + blockLight, 0));
	nightRatio = 1.0 - (skyLight / sum);
	dayLight = getSkyColor(timeOfDay);

	varColor = calculateLighting(timeOfDay, skyLight, blockLight, ao);

	vec3 lightColor = (max(dayLight * skyLight, blockColor * blockLight - vec3(color.a*2.0)) + ambientColor) * ao;
	emissionLight = step(0.001, color.a) * (1.0 - luminance(lightColor));

	hwColor = inAux / 255.0;


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
