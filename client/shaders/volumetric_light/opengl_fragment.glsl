#include<lighting>
#include<noise>

#define rendered texture0
#define depthmap texture1

uniform sampler2D depthmap;

uniform vec3 sunPositionScreen;
uniform float sunBrightness;
uniform vec3 moonPositionScreen;
uniform float moonBrightness;

uniform lowp float volumetricLightStrength;
uniform vec3 lightDir;
uniform float timeOfDay;

CENTROID_ in mediump vec2 varTexCoord;

float sampleVolumetricLight(vec2 uv, vec3 lightVec, float rawDepth)
{
	lightVec = 0.5 * lightVec / lightVec.z + 0.5;
	const float samples = 32.;

	float initialAlpha = texture2D(rendered, uv).a;
	float result = (rawDepth >= 1.0) ? 1.0 : (1.0 - initialAlpha);

	float bias = white_noise_3d(vec3(uv, rawDepth));
	vec2 samplepos;

	const float DECAY = 0.96;
	float illumination_decay = 1.0;

	for (float i = 1.; i < samples; i++) {
		samplepos = mix(uv, lightVec.xy, ((i + bias) / samples) * 0.8);

		if (min(samplepos.x, samplepos.y) > 0. && max(samplepos.x, samplepos.y) < 1.) {
			float sampleDepth = texture2D(depthmap, samplepos).r;
			float sampleAlpha = texture2D(rendered, samplepos).a;

			vec3 sampleColor = texture2D(rendered, samplepos).rgb;
			float brightness = max(sampleColor.r, max(sampleColor.g, sampleColor.b));

			if (sampleDepth >= 1.0 || brightness > 0.65) {
				if (sampleDepth >= 1.0) {
					result += 1.0 * illumination_decay;
				} else {
					result += (1.0 - sampleAlpha) * illumination_decay;
				}
			}
		}
		illumination_decay *= DECAY;
	}

	float depthFactor = mix(1.0 - initialAlpha, 1.0, smoothstep(0.0, 0.95, rawDepth));
	return (result / samples) * max(0.1, depthFactor);
}

vec3 getDirectLightScatteringAtGround()
{
	// Based on talk at 2002 Game Developers Conference by Naty Hoffman and Arcot J. Preetham
	const float beta_r0 = 1e-5; // Rayleigh scattering beta

	// These factors are calculated based on expected value of scattering factor of 1e-5
	// for Nitrogen at 532nm (green), 2e25 molecules/m3 in atmosphere
	const vec3 beta_r0_l = vec3(3.3362176e-01, 8.75378289198826e-01, 1.95342379700656) * beta_r0; // wavelength-dependent scattering

	const float atmosphere_height = 15000.; // height of the atmosphere in meters
	// sun/moon light at the ground level, after going through the atmosphere
	return exp(-beta_r0_l * atmosphere_height / (1e-5 - dot(lightDir, vec3(0., 1., 0.))));
}

vec3 applyVolumetricLight(vec3 color, vec2 uv, float rawDepth)
{
	vec3 lookDirection = normalize(vec3(uv.x * 2. - 1., uv.y * 2. - 1., 1.0));

	const float boost = 3.5;
	float brightness = 0.;
	vec3 sourcePosition = vec3(-1., -1., -1);

	if (sunPositionScreen.z > 0. && sunBrightness > 0.) {
		brightness = sunBrightness;
		sourcePosition = sunPositionScreen;
	}
	else if (moonPositionScreen.z > 0. && moonBrightness > 0.) {
		brightness = moonBrightness * 0.05;
		sourcePosition = moonPositionScreen;
	}

	float dotLightView = max(0., dot(sourcePosition, lookDirection));
	float sunGlowFactor = pow(dotLightView, 160.0);
	float rayAngleFactor = pow(dotLightView, 4.0);

	float volumetricSample = sampleVolumetricLight(uv, sourcePosition, rawDepth);

	float lightFactor = brightness * volumetricSample * (sunGlowFactor * 0.3 + rayAngleFactor * 0.7);

	lightFactor *= volumetricLightStrength * 5.0;

	vec3 scatteringColor = boost * getDirectLightScatteringAtGround() * getSkyColor(timeOfDay);

	color = mix(color, color + scatteringColor, clamp(lightFactor, 0.0, 0.85));
	color = 1.0 - exp(-color * 1.4);

	return color;
}

void main(void)
{
	vec2 uv = varTexCoord.st;
	vec3 color = texture2D(rendered, uv).rgb;
	// translate to linear colorspace (approximate)
	color = srgb_to_linear(color);

	if (volumetricLightStrength > 0.0) {
		float rawDepth = texture2D(depthmap, uv).r;

		color = applyVolumetricLight(color, uv, rawDepth);
	}

	outputColor(vec4(color, 1.0)); // force full alpha to avoid holes in the image.
}
