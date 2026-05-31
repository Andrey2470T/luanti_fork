#include<common>
#include<lighting>

uniform lowp vec4 materialColor;

out vec3 vNormal;
out vec3 vPosition;
out vec3 worldPosition;
out lowp vec3 varColor;
out lowp vec3 dayLight;

CENTROID_ out mediump vec2 varTexCoord;

#ifdef ENABLE_DYNAMIC_SHADOWS
	out float cosLight;
	out float adj_shadow_strength;
	out float f_normal_length;
	out vec3 shadow_position;
	out float perspective_factor;
#endif

out highp vec3 eyeVec;
out float nightRatio;
out float vIDiff;

float directional_ambient(vec3 normal)
{
	vec3 v = normal * normal;

	if (normal.y < 0.0)
		return dot(v, vec3(0.670820, 0.447213, 0.836660));

	return dot(v, vec3(0.670820, 1.000000, 0.836660));
}

void main(void)
{
	varTexCoord = (mTexture * vec4(inTexCoord0.xy, 1.0, 1.0)).st;
	gl_Position = mWorldViewProj * vec4(inPosition, 1.0);

	vPosition = gl_Position.xyz;
	vNormal = (mWorld * vec4(inNormal, 0.0)).xyz;
	worldPosition = (mWorld * vec4(inPosition, 1.0)).xyz;
	eyeVec = -(mWorldView * vec4(inPosition, 1.0)).xyz;

#if (MATERIAL_TYPE == TILE_MATERIAL_PLAIN) || (MATERIAL_TYPE == TILE_MATERIAL_PLAIN_ALPHA)
	vIDiff = 1.0;
#else
	// This is intentional comparison with zero without any margin.
	// If normal is not equal to zero exactly, then we assume it's a valid, just not normalized vector
	vIDiff = length(inNormal) == 0.0
		? 1.0
		: directional_ambient(normalize(inNormal));
#endif

	// Calculating the light color
	vec4 color = inColor;
	color *= materialColor;
	float skyLight = color.r;
	float blockLight = color.g;
	float sum = float(max(skyLight + blockLight, 0));
	nightRatio = 1.0 - (skyLight / sum);
	dayLight = getSkyColor();
	varColor = calculateLighting(skyLight, blockLight, 1.0);

#ifdef ENABLE_DYNAMIC_SHADOWS
	vertexStage(
		vec4(inPosition, 1.0), vNormal, f_normal_length, cosLight,
		shadow_position, perspective_factor, adj_shadow_strength
	);
#endif
}
