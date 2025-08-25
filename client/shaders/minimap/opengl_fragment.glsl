#define baseTexture mTexture0
#define heightMap mTexture1

uniform sampler2D baseTexture;
uniform sampler2D heightMap;
uniform vec3 mYawVec;

in vec4 vVertexColor;
in vec2 vUV0;

out vec4 outColor;

void main (void)
{
	//texture sampling rate
	const float step = 1.0 / 256.0;
	float tl = texture2D(heightMap, vec2(vUV0.x - step, vUV0.y + step)).r;
	float t  = texture2D(heightMap, vec2(vUV0.x,        vUV0.y + step)).r;
	float tr = texture2D(heightMap, vec2(vUV0.x + step, vUV0.y + step)).r;
	float r  = texture2D(heightMap, vec2(vUV0.x + step, vUV0.y       )).r;
	float br = texture2D(heightMap, vec2(vUV0.x + step, vUV0.y - step)).r;
	float b  = texture2D(heightMap, vec2(vUV0.x,        vUV0.y - step)).r;
	float bl = texture2D(heightMap, vec2(vUV0.x - step, vUV0.y - step)).r;
	float l  = texture2D(heightMap, vec2(vUV0.x - step, vUV0.y       )).r;
	float dX = (tr + 2.0 * r + br) - (tl + 2.0 * l + bl);
	float dY = (bl + 2.0 * b + br) - (tl + 2.0 * t + tr);
	vec4 bump = vec4 (normalize(vec3 (dX, dY, 0.1)),1.0);
	float height = 2.0 * texture2D(heightMap, vec2(vUV0.x, vUV0.y)).r - 1.0;
	vec4 base = texture2D(baseTexture, vUV0).rgba;
	vec3 L = normalize(vec3(0.0, 0.75, 1.0));
	float specular = pow(clamp(dot(reflect(L, bump.xyz), mYawVec), 0.0, 1.0), 1.0);
	float diffuse = dot(mYawVec, bump.xyz);

	outColor = vec4(vec3(1.1 * diffuse + 0.05 * height + 0.5 * specular) * base.rgb, base.a);
	outColor *= vVertexColor;
}
