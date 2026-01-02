#ifndef ACES_HLSL
#define ACES_HLSL

#include "Common.hlsli"

#define ACEScc_MAX      1.4679964
#define ACEScc_MIDGRAY  0.4135884

#define RRT_GLOW_GAIN   0.05
#define RRT_GLOW_MID    0.08

#define RRT_RED_SCALE   0.82
#define RRT_RED_PIVOT   0.03
#define RRT_RED_HUE     0.0
#define RRT_RED_WIDTH   135.0
#define RRT_SAT_FACTOR  0.96

#define ODT_SAT_FACTOR  0.93

// Precomputed matrices (pre-transposed)
// See https://github.com/ampas/aces-dev/blob/master/transforms/ctl/README-MATRIX.md
static const half3x3 sRGB_2_AP0 = {
    0.4397010, 0.3829780, 0.1773350,
    0.0897923, 0.8134230, 0.0967616,
    0.0175440, 0.1115440, 0.8707040
};

static const half3x3 sRGB_2_AP1 = {
    0.61319, 0.33951, 0.04737,
    0.07021, 0.91634, 0.01345,
    0.02062, 0.10957, 0.86961
};

static const half3x3 AP0_2_sRGB = {
    2.52169, -1.13413, -0.38756,
    -0.27648, 1.37272, -0.09624,
    -0.01538, -0.15298, 1.16835,
};

static const half3x3 AP1_2_sRGB = {
    1.70505, -0.62179, -0.08326,
    -0.13026, 1.14080, -0.01055,
    -0.02400, -0.12897, 1.15297,
};

static const half3x3 AP0_2_AP1_MAT = {
     1.4514393161, -0.2365107469, -0.2149285693,
    -0.0765537734,  1.1762296998, -0.0996759264,
     0.0083161484, -0.0060324498,  0.9977163014
};

static const half3x3 AP1_2_AP0_MAT = {
     0.6954522414, 0.1406786965, 0.1638690622,
     0.0447945634, 0.8596711185, 0.0955343182,
    -0.0055258826, 0.0040252103, 1.0015006723
};

static const half3x3 AP1_2_XYZ_MAT = {
     0.6624541811, 0.1340042065, 0.1561876870,
     0.2722287168, 0.6740817658, 0.0536895174,
    -0.0055746495, 0.0040607335, 1.0103391003
};

static const half3x3 XYZ_2_AP1_MAT = {
     1.6410233797, -0.3248032942, -0.2364246952,
    -0.6636628587,  1.6153315917,  0.0167563477,
     0.0117218943, -0.0082844420,  0.9883948585
};

static const half3x3 XYZ_2_REC709_MAT = {
     3.2409699419, -1.5373831776, -0.4986107603,
    -0.9692436363,  1.8759675015,  0.0415550574,
     0.0556300797, -0.2039769589,  1.0569715142
};

static const half3x3 XYZ_2_REC2020_MAT = {
     1.7166511880, -0.3556707838, -0.2533662814,
    -0.6666843518,  1.6164812366,  0.0157685458,
     0.0176398574, -0.0427706133,  0.9421031212
};

static const half3x3 XYZ_2_DCIP3_MAT = {
     2.7253940305, -1.0180030062, -0.4401631952,
    -0.7951680258,  1.6897320548,  0.0226471906,
     0.0412418914, -0.0876390192,  1.1009293786
};

static const half3 AP1_RGB2Y = half3(0.272229, 0.674082, 0.0536895);

static const half3x3 RRT_SAT_MAT = {
    0.9708890, 0.0269633, 0.00214758,
    0.0108892, 0.9869630, 0.00214758,
    0.0108892, 0.0269633, 0.96214800
};

static const half3x3 ODT_SAT_MAT = {
    0.949056, 0.0471857, 0.00375827,
    0.019056, 0.9771860, 0.00375827,
    0.019056, 0.0471857, 0.93375800
};

static const half3x3 D60_2_D65_CAT = {
     0.98722400, -0.00611327, 0.0159533,
    -0.00759836,  1.00186000, 0.0053302,
     0.00307257, -0.00509595, 1.0816800
};

// ACES Color Space Conversion - ACES to ACEScg
// converts ACES2065-1 (AP0 w/ linear encoding) to
// ACEScg (AP1 w/ linear encoding)
half3 ACES_to_ACEScg(half3 x)
{
    return mul(AP0_2_AP1_MAT, x);
}

half RgbToSaturation(half3 rgb)
{
    const half TINY = 1e-4;
	half mi = min(rgb.r, min(rgb.g, rgb.b));
	half ma = max(rgb.r, max(rgb.g, rgb.b));
    return (max(ma, TINY) - max(mi, TINY)) / max(ma, 1e-2);
}

half RgbToYc(half3 rgb)
{
    const half ycRadiusWeight = 1.75;

    // Converts RGB to a luminance proxy, here called Yc
    // Yc is ~ Y + K * Chroma
    // Constant Yc is a cone-shaped surface in RGB space, with the tip on the
    // neutral axis, towards white.
    // Yc is normalized: RGB 1 1 1 maps to Yc = 1
    //
    // ycRadiusWeight defaults to 1.75, although can be overridden in function
    // call to RgbToYc
    // ycRadiusWeight = 1 -> Yc for pure cyan, magenta, yellow == Yc for neutral
    // of same value
    // ycRadiusWeight = 2 -> Yc for pure red, green, blue  == Yc for  neutral of
    // same value.

    half r = rgb.x;
    half g = rgb.y;
    half b = rgb.z;
    half chroma = sqrt(b * (b - g) + g * (g - r) + r * (r - b));
    return (b + g + r + ycRadiusWeight * chroma) / 3.0;
}

half RgbToHue(half3 rgb)
{
    // Returns a geometric hue angle in degrees (0-360) based on RGB values.
    // For neutral colors, hue is undefined and the function will return a quiet NaN value.
    half hue;
    if (rgb.x == rgb.y && rgb.y == rgb.z)
        hue = 0.0; // RGB triplets where RGB are equal have an undefined hue
    else
        hue = (180.0 / PI) * atan2(sqrt(3.0) * (rgb.y - rgb.z), 2.0 * rgb.x - rgb.y - rgb.z);

    if (hue < 0.0) hue = hue + 360.0;

    return hue;
}

half CenterHue(half hue, half centerH)
{
    half hueCentered = hue - centerH;
    if (hueCentered < -180.0) hueCentered = hueCentered + 360.0;
    else if (hueCentered > 180.0) hueCentered = hueCentered - 360.0;
    return hueCentered;
}

half SigmoidShaper(half x)
{
    // Sigmoid function in the range 0 to 1 spanning -2 to +2.
    half t = max(1.0 - abs(x / 2.0), 0.0);
    half y = 1.0 + FastSign(x) * (1.0 - t * t);

    return y / 2.0;
}

half GlowFwd(half ycIn, half glowGainIn, half glowMid)
{
    half glowGainOut;

    if (ycIn <= 2.0 / 3.0 * glowMid)
        glowGainOut = glowGainIn;
    else if (ycIn >= 2.0 * glowMid)
        glowGainOut = 0.0;
    else
        glowGainOut = glowGainIn * (glowMid / ycIn - 1.0 / 2.0);

    return glowGainOut;
}

half3 XYZ_2_xyY(half3 XYZ)
{
	half divisor = max(dot(XYZ, (1.0).xxx), 1e-4);
	return half3(XYZ.xy / divisor, XYZ.y);
}

half3 xyY_2_XYZ(half3 xyY)
{
	half m = xyY.z / max(xyY.y, 1e-4);
	half3 XYZ = half3(xyY.xz, (1.0 - xyY.x - xyY.y));
	XYZ.xz *= m;
	return XYZ;
}

#define DIM_SURROUND_GAMMA 0.9811
half3 DarkSurroundToDimSurround(half3 linearCV)
{
    half3 XYZ = mul(AP1_2_XYZ_MAT, linearCV);

    half3 xyY = XYZ_2_xyY(XYZ);
    xyY.z = clamp(xyY.z, 0.0, HALF_MAX);
    xyY.z = pow(xyY.z, DIM_SURROUND_GAMMA);
    XYZ = xyY_2_XYZ(xyY);

    return mul(XYZ_2_AP1_MAT, XYZ);
}
#endif // ACES_HLSL