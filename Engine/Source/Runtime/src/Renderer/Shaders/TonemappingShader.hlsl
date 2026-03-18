#include "ACES.hlsli"
#include "Common.hlsli"
#include "Colors.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::TonemapUniforms, uniforms);

// Neutral tonemapping (Hable/Hejl/Frostbite)
float3 NeutralCurve(float3 x, float a, float b, float c, float d, float e, float f)
{
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

float3 NeutralTonemap(float3 x)
{
    // Tonemap
    float a = 0.2;
    float b = 0.29;
    float c = 0.24;
    float d = 0.272;
    float e = 0.02;
    float f = 0.3;
    float whiteLevel = 5.3;
    float whiteClip = 1.0;

    float3 whiteScale = (1.0).xxx / NeutralCurve(whiteLevel, a, b, c, d, e, f);
    x = NeutralCurve(x * whiteScale, a, b, c, d, e, f);
    x *= whiteScale;

    // Post-curve white point adjustment
    x /= whiteClip.xxx;

    return x;
}

// Filmic tonemapping (ACES fitting)
// Input is ACES2065-1 (AP0 w/ linear encoding)
float3 AcesTonemap(float3 aces)
{
    // Glow module
    float saturation = RgbToSaturation(aces);
    float ycIn = RgbToYc(aces);
    float s = SigmoidShaper((saturation - 0.4) / 0.2);
    float addedGlow = 1.0 + GlowFwd(ycIn, RRT_GLOW_GAIN * s, RRT_GLOW_MID);
    aces *= addedGlow;

    // Red modifier
    float hue = RgbToHue(aces);
    float centeredHue = CenterHue(hue, RRT_RED_HUE);
    float hueWeight;
    {
        hueWeight = smoothstep(0.0, 1.0, 1.0 - abs(2.0 * centeredHue / RRT_RED_WIDTH));
        hueWeight *= hueWeight;
    }

    aces.r += hueWeight * saturation * (RRT_RED_PIVOT - aces.r) * (1.0 - RRT_RED_SCALE);

    // ACES to RGB rendering space
    float3 acescg = max(0.0, ACES_to_ACEScg(aces));

    // Global desaturation
    acescg = lerp(dot(acescg, AP1_RGB2Y).xxx, acescg, RRT_SAT_FACTOR.xxx);

    // Luminance fitting of *RRT.a1.0.3 + ODT.Academy.RGBmonitor_100nits_dim.a1.0.3*.
    // RMSE: 0.0012846272106
    const float a = 278.5085;
    const float b = 10.7772;
    const float c = 293.6045;
    const float d = 88.7122;
    const float e = 80.6889;
    float3 x = acescg;
    float3 rgbPost = (x * (a * x + b)) / (x * (c * x + d) + e);

    // Apply gamma adjustment to compensate for dim surround
    float3 linearCV = DarkSurroundToDimSurround(rgbPost);

    // Apply desaturation to compensate for luminance difference
    linearCV = lerp(dot(linearCV, AP1_RGB2Y).xxx, linearCV, ODT_SAT_FACTOR.xxx);

    // Convert to display primary encoding
    // Rendering space RGB to XYZ
    float3 XYZ = mul(AP1_2_XYZ_MAT, linearCV);

    // Apply CAT from ACES white point to assumed observer adapted white point
    XYZ = mul(D60_2_D65_CAT, XYZ);

    // CIE XYZ to display primaries
    linearCV = mul(XYZ_2_REC709_MAT, XYZ);

    return linearCV;
}

[shader("pixel")]
float4 tonemappingFragmentShader(FScreenVertexOutput IN) : SV_TARGET
{
    Texture2D<float4> sceneTexture = ResourceDescriptorHeap[uniforms.sceneColor];
    float4 color = sceneTexture.Sample(Sampler_Point_Clamp, IN.texCoord);
    color.rgb = LinearTosRGB(NeutralTonemap(color.rgb));
    return color;
}
