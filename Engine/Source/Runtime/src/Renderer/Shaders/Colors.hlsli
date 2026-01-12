#ifndef COLORS_HLSL
#define COLORS_HLSL

half Luminance(half3 linearRgb)
{
    return dot(linearRgb, float3(0.2126729, 0.7151522, 0.0721750));
}

half Luminance(half4 linearRgba)
{
    return Luminance(linearRgba.rgb);
}

half3 LinearTosRGB(half3 lin)
{
    return select(lin < 0.00313067, lin * 12.92, pow(lin, (1.0/2.4)) * 1.055 - 0.055);
}

half3 sRGBToLinear(half3 color)
{
    color = max(6.10352e-5, color); // minimum positive non-denormal (fixes black problem on DX11 AMD and NV)
    return select(color > 0.04045, pow(color * (1.0 / 1.055) + 0.0521327, 2.4), color * (1.0 / 12.92));
}
#endif // COLORS_HLSL