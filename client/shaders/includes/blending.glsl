#define NO_BLEND_MODE 0
#define ALPHA_BLEND_MODE 1
#define ADD_BLEND_MODE 2
#define SUBTRACT_BLEND_MODE 3
#define REVSUBTRACT_BLEND_MODE 4
#define MULTIPLY_BLEND_MODE 5
#define SCREEN_BLEND_MODE 6
#define OVERLAY_BLEND_MODE 7
#define MIN_BLEND_MODE 8
#define MAX_BLEND_MODE 9

// All functions below use the same blending for rgb and alpha (separated one is not supported)

vec4 AlphaBlend(vec4 src, vec4 dst)
{
	return src * src.a + dst * (1 - src.a);
}

vec4 AddBlend(vec4 src, vec4 dst)
{
	return src + dst;
}

vec4 SubtractBlend(vec4 src, vec4 dst)
{
	return src - dst;
}

vec4 RevSubtractBlend(vec4 src, vec4 dst)
{
	return dst - src;
}

vec4 MultiplyBlend(vec4 src, vec4 dst)
{
	return mix(src, src * dst, dst.a);
}

vec4 ScreenBlend(vec4 src, vec4 dst)
{
	return mix(src, vec4(1.0) - (vec4(1.0) - src) * (vec4(1.0) - dst), dst.a);
}

#define OVERLAY_CHANNEL(src, dst) \
	src < 0.5 ? (2.0 * src * dst) : (1.0 - 2.0*(1.0 - src)*(1.0 - dst));

vec4 OverlayBlend(vec4 src, vec4 dst)
{
	vec3 res = src;
	res.r = OVERLAY_CHANNEL(src.r, dst.r);
	res.g = OVERLAY_CHANNEL(src.g, dst.g);
	res.b = OVERLAY_CHANNEL(src.b, dst.b);

	return mix(src, res, dst.a);
}

vec4 MinBlend(vec4 src, vec4 dst)
{
	return min(src, dst);
}

vec4 MaxBlend(vec4 src, vec4 dst)
{
	return max(src, dst);
}

vec4 DoBlend(int blendMode, vec4 src, vec4 dst)
{
	if (blendMode == 0)
		return dst; // no blending
	else if (blendMode == 1)
		return AlphaBlend(src, dst);
	else if (blendMode == 2)
		return AddBlend(src, dst);
	else if (blendMode == 3)
		return SubtractBlend(src, dst);
	else if (blendMode == 4)
		return RevSubtractBlend(src, dst);
	else if (blendMode == 5)
		return MultiplyBlend(src, dst);
	else if (blendMode == 6)
		return ScreenBlend(src, dst);
	else if (blendMode == 7)
		return OverlayBlend(src, dst);
	else if (blendMode == 8)
		return MinBlend(src, dst);
	else if (blendMode == 9)
		return MaxBlend(src, dst);

	return dst;
}
