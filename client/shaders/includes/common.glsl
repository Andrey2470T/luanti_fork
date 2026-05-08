uniform float mGamma = 2.2;

vec4 srgb_to_linear(vec4 color, float gamma)
{
  return pow(color, vec4(gamma));
}

vec4 linear_to_srgb(vec4 color, float gamma)
{
  return pow(color, vec4(1.0/gamma));
}

vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}