#ifndef MESH_LIGHTING_HLSL
#define MESH_LIGHTING_HLSL

#include "BRDF.hlsli"
#include "SurfaceShading.hlsli"
#include "Atmosphere/SkyAtmosphereCommon.hlsli"

float3 EvaluateMeshLighting(Gleam::SurfaceOutput surface,
                            float3 worldPosition,
                            float3 worldNormal,
                            float3 viewDir,
                            uint2 pixelCoord,
                            Gleam::ShaderResourceIndex brdfTexture,
                            Gleam::ShaderResourceIndex ggxEssTexture,
                            Gleam::ShaderResourceIndex ggxEAvgTexture,
                            Gleam::ShaderResourceIndex diffuseReflectionTexture,
                            Gleam::ShaderResourceIndex specularReflectionTexture,
                            Gleam::ShaderResourceIndex shadowTexture)
{
    DirectLight light;
    if (atmosphereUniforms.transmittanceLutTexture != InvalidResourceIndex && atmosphereUniforms.multiScatterLutTexture != InvalidResourceIndex)
    {
        light.direction = atmosphereUniforms.sunDirection;
        light.illuminance = GetSunLuminance(GetSkyWorldPosition(worldPosition), atmosphereUniforms.sunDirection);
    }
    else
    {
        light.direction = atmosphereUniforms.sunDirection;
        light.illuminance = atmosphereUniforms.sunIlluminance;
    }

    float shadowVisibility = 1.0f;
    if (shadowTexture != InvalidResourceIndex)
    {
        Texture2D<unorm float> shadowTex = ResourceDescriptorHeap[shadowTexture];
        shadowVisibility = shadowTex.Load(int3(pixelCoord, 0));
    }

    float3 color = surface.emission.rgb;
    color += EvaluateDirectLight(surface,
                                 ggxEssTexture,
                                 ggxEAvgTexture,
                                 light,
                                 viewDir,
                                 worldNormal) * shadowVisibility;
    color += EvaluateIndirectLight(surface,
                                   brdfTexture,
                                   ggxEssTexture,
                                   ggxEAvgTexture,
                                   diffuseReflectionTexture,
                                   specularReflectionTexture,
                                   viewDir,
                                   worldNormal);
    return color;
}

#endif // MESH_LIGHTING_HLSL
