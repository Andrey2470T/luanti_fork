//
// Simple, fast noise function.
// See: https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
//
vec4 perm(vec4 x)
{
	return mod(((x * 34.0) + 1.0) * x, 289.0);
}

float snoise(vec3 p)
{
	vec3 a = floor(p);
	vec3 d = p - a;
	d = d * d * (3.0 - 2.0 * d);

	vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
	vec4 k1 = perm(b.xyxy);
	vec4 k2 = perm(k1.xyxy + b.zzww);

	vec4 c = k2 + a.zzzz;
	vec4 k3 = perm(c);
	vec4 k4 = perm(c + 1.0);

	vec4 o1 = fract(k3 * (1.0 / 41.0));
	vec4 o2 = fract(k4 * (1.0 / 41.0));

	vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
	vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

	return o4.y * d.y + o4.x * (1.0 - d.y);
}

// Corresponding gradient of snoise
vec3 gnoise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    vec3 dd = 6.0 * d * (1.0 - d);
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    vec4 dz1 = (o2 - o1) * dd.z;
    vec2 dz2 = dz1.yw * d.x + dz1.xz * (1.0 - d.x);

    vec2 dx = (o3.yw - o3.xz) * dd.x;

    return vec3(
        dx.y * d.y + dx.x * (1. - d.y),
        (o4.y - o4.x) * dd.y,
        dz2.y * d.y + dz2.x * (1. - d.y)
    );
}

vec2 wave_noise(vec3 p, float off) {
	return (gnoise(p + vec3(0.0, 0.0, off)) * 0.4 + gnoise(2.0 * p + vec3(0.0, off, off)) * 0.2 + gnoise(3.0 * p + vec3(0.0, off, off)) * 0.225 + gnoise(4.0 * p + vec3(-off, off, 0.0)) * 0.2).xz;
}