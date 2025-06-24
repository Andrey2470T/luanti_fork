#version 320 core

#define BM_ALPHA 0
#define BM_ADD 1
#define BM_SUBTRACTION 2
#define BM_MULTIPLY 3
#define BM_DIVISION 4
#define BM_SCREEN 5
#define BM_OVERLAY 6
#define BM_HARD_LIGHT 7
#define BM_SOFT_LIGHT 8
#define BM_GRAIN_EXTRACT 9
#define BM_GRAIN_MERGE 10
#define BM_DARKEN_ONLY 11
#define BM_LIGHTEN_ONLY 12

uniform int uBlendMode;
uniform int uTextureUsage;
uniform sampler2D uFramebuffer;
uniform sampler2D uTexture;

layout (std140) uniform fogParams {
    int enable;
    int type;
    vec4 color;
    float start;
    float end;
    float density;
};

in vec2 vTextureCoord0;
in vec4 vVertexColor;
in float vFogCoord;

float computeFog()
{
	const float LOG2 = 1.442695;
	float FogFactor = 0.0;

	if (uFogType == 0) // Exp
	{
		FogFactor = exp2(-fogParams.density * vFogCoord * LOG2);
	}
	else if (fogParams.type == 1) // Linear
	{
		float Scale = 1.0 / (fogParams.end - fogParams.start);
		FogFactor = (fogParams.end - vFogCoord) * Scale;
	}
	else if (fogParams.type == 2) // Exp2
	{
		FogFactor = exp2(-fogParams.density * fogParams.density * vFogCoord * vFogCoord * LOG2);
	}

	FogFactor = clamp(FogFactor, 0.0, 1.0);

	return FogFactor;
}

vec4 AlphaBlend(vec4 src, vec4 dst)
{
    vec4 color;
    if (dst.a == 1.0) {
        color = vec4(src.rgb * src.a + dst.rgb * (1.0 - src.a), dst.a);
    }
    else {
        f32 resA = src.a + dst.a * (1.0 - src.a);

        if (resA == 0)
            return vec4(0, 0, 0, 0);

        color = vec4((src.rgb * src.a + dst.rgb * dst.a * (1.0 - src.a)) / resA, resA);
    }
    
    return color;
}

vec4 AddBlend(vec4 src, vec4 dst)
{
	return src + dst;
}

vec4 SubtractBlend(vec4 src, vec4 dst)
{
	return src - dst;
}

vec4 MultiplyBlend(vec4 src, vec4 dst)
{
    return src * dst;
}

vec4 DivisionBlend(vec4 src, vec4 dst)
{
    return src / dst;
}

vec4 ScreenBlend(vec4 src, vec4 dst)
{
    return vec4(vec3(1.0) - (vec3(1.0) - src.rgb)*(vec3(1.0) - dst.rgb), dst.a);
}

vec4 OverlayBlend(vec4 src, vec4 dst)
{
	T max_v = T_MAX(T);

	vec4 color = vec4(0, 0, 0, dst.a);

	if (src.r <  0.5)
        color.r = 2 * src.r * dst.r;
	else
        color.r = 1.0 - 2*(1.0 - src.r)*(1.0 - dst.r);
	if (src.g < 0.5)
        color.g = 2 * src.g * dst.g;
	else
        color.g = 1.0 - 2*(1.0 - src.g)*(1.0 - dst.g);
	if (src.b < 0.5)
        color.b = 2 * src.b * dst.b;
	else
        color.b = 1.0 - 2*(1.0 - src.b)*(1.0 - dst.b);

	return color;
}

vec4 HardLightBlend(vec4 src, vec4 dst)
{
	T max_v = T_MAX(T);

    ColorRGBA<T> newColor(src.getFormat(), 0, 0, 0, dst.A());
	vec4 color = vec4(0, 0, 0, dst.a);

	if (dst.r <  0.5)
        color.r = 2 * src.r * dst.r;
	else
        color.r = 1.0 - 2*(1.0 - src.r)*(1.0 - dst.r);
	if (dst.g < 0.5)
        color.g = 2 * src.g * dst.g;
	else
        color.g = 1.0 - 2*(1.0 - src.g)*(1.0 - dst.g);
	if (dst.b < 0.5)
        color.b = 2 * src.b * dst.b;
	else
        color.b = 1.0 - 2*(1.0 - src.b)*(1.0 - dst.b);

	return color;
}

vec4 SoftLightBlend(vec4 src, vec4 dst)
{
    return vec4((vec3(1.0) - dst.rgb*2)*src.rgb*src.rgb + dst.rgb*src.rgb*2, dst.a);
}

vec4 GrainExtractBlend(vec 4src, vec4 dst)
{
    return vec4(dst.rgb - src.rgb + 0.5, dst.a);
}

vec4 GrainMergeBlend(const ColorRGBA<T> &src, const ColorRGBA<T> &dst)
{
    return vec4(dst.rgb + src.rgb - 0.5, dst.a);
}

vec4 DarkenOnlyBlend(vec4 src, vec4 dst)
{
	return vec4(min(src, dst), dst.a);
}

vec4 LightenOnlyBlend(vec4 src, vec4 dst)
{
	return vec4(max(src, dst), dst.a);
}

vec4 DoBlend(vec4 src, vec4 dst)
{
    if (uBlendMode == 0)
        return AlphaBlend(src, dst);
    else if (uBlendMode == 1)
        return AddBlend(src, dst);
    else if (uBlendMode == 2)
        return SubtractBlend(src, dst);
    else if (uBlendMode == 3)
        return MultiplyBlend(src, dst);
    else if (uBlendMode == 4)
        return DivisionBlend(src, dst);
    else if (uBlendMode == 5)
        return SceeenBlend(src, dst);
    else if (uBlendMode == 6)
        return OverlayBlend(src, dst);
    else if (uBlendMode == 7)
        return HardLightBlend(src, dst);
    else if (uBlendMode == 8)
        return SoftLightBlend(src, dst);
    else if (uBlendMode == 9)
        return GrainExtractBlend(src, dst);
    else if (uBlendMode == 10)
        return GrainMergeBlend(src, dst);
    else if (uBlendMode == 11)
        return DarkenOnlyBlend(src, dst);
    else if (uBlendMode == 12)
        return LightOnlyBlend(src, dst);
        
    return vec4(0.0);
}

void main()
{
	vec4 Color = vVertexColor;

	if (bool(uTextureUsage0)) {
		Color *= texture2D(uTexture, vTextureCoord0);
	}
	
	if (bool(fogParams.enable))
	{
		float FogFactor = computeFog();
		vec4 FogColor = fogParams.color;
		FogColor.a = 1.0;
		Color = mix(FogColor, Color, FogFactor);
	}
	
	Color = DoBlend(Color, texture2D(uFramebuffer, vTextureCoord));

	gl_FragColor = Color;
}