#ifndef FFX_CLASSIFIER_SHADOWS_CALLBACKS_HLSL
#define FFX_CLASSIFIER_SHADOWS_CALLBACKS_HLSL

#include "Common.hlsli"
#include "ShaderTypes.h"
#include "FidelityFXCore.hlsli"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);
CONSTANT_BUFFER(Gleam::SkyAtmosphereUniforms, atmosphereUniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
CONSTANT_BUFFER(Gleam::SkyAtmosphereParameters, atmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
PUSH_CONSTANT(Gleam::RayTracedSunShadowClassificationConstants, constants);

FfxFloat32x4 TextureSize()
{
    return FfxFloat32x4(camera.resolution.x, camera.resolution.y, 1.0f / camera.resolution.x, 1.0f / camera.resolution.y);
}

FfxFloat32x3 LightDir()
{
    return -atmosphereUniforms.sunDirection;
}

FfxUInt32 TileTolerance()
{
    return constants.tileTolerance;
}

FfxFloat32 FfxClassifierSampleDepth(FfxUInt32x2 pixelCoord)
{
    Texture2D<float> depthTexture = ResourceDescriptorHeap[constants.depthTexture];
    return depthTexture[pixelCoord];
}

FfxFloat32x3 FfxClassifierSampleNormal(FfxUInt32x2 pixelCoord)
{
    Texture2D<float2> normalTexture = ResourceDescriptorHeap[constants.normalTexture];
    return OctDecode(normalTexture[pixelCoord]);
}

FfxUInt32 CountBits(FfxUInt32 mask)
{
    return countbits(mask);
}

void FfxClassifierStoreTile(FfxUInt32x4 tile)
{
    RWByteAddressBuffer tileBuffer = ResourceDescriptorHeap[constants.tileBuffer];
    RWByteAddressBuffer tileCountBuffer = ResourceDescriptorHeap[constants.tileCountBuffer];

    FfxUInt32 index;
    tileCountBuffer.InterlockedAdd(0, 1, index);
    tileBuffer.Store4(index * sizeof(uint4), tile);
}

void FfxClassifierStoreLightMask(FfxUInt32x2 tileCoord, FfxUInt32 lightMask)
{
    RWTexture2D<uint> rayHitTexture = ResourceDescriptorHeap[constants.rayHitTexture];
    rayHitTexture[tileCoord] = ~lightMask;
}

FfxFloat32 SkyHeight()
{
    return atmosphereParams.topRadius;
}

FfxFloat32 SunSizeLightSpace()
{
    const FfxFloat32 sunHalfAngle = 0.5f * atmosphereUniforms.sunAngularDiameter * (PI / 180.0f);
    return tan(sunHalfAngle);
}

FfxFloat32Mat4 ViewToWorld()
{
    return camera.invViewProjectionMatrix;
}

// Cascade-blocking hooks are unused for ray-traced shadows (classifier mode 0 passes useCascadeBlocking=false),
// but the algorithm still references them, so provide compilable stubs.
FfxUInt32 CascadeCount() { return 0u; }
FfxFloat32 BlockerOffset() { return 0.0f; }
FfxFloat32 CascadeSize() { return 0.0f; }
FfxBoolean RejectLitPixels() { return false; }
FfxBoolean UseCascadesForRayT() { return false; }
FfxFloat32x4 CascadeScale(FfxInt32 index) { return FfxFloat32x4(0.0f, 0.0f, 0.0f, 0.0f); }
FfxFloat32x4 CascadeOffset(FfxInt32 index) { return FfxFloat32x4(0.0f, 0.0f, 0.0f, 0.0f); }
FfxFloat32Mat4 LightView() { return (FfxFloat32Mat4)0; }
FfxFloat32Mat4 InverseLightView() { return (FfxFloat32Mat4)0; }
FfxFloat32 FfxClassifierSampleShadowMap(FfxFloat32x2 sampleUV, FfxUInt32 cascadeIndex) { return 0.0f; }

static const FfxFloat32x2 k_poissonDisc[] = {
    FfxFloat32x2(0.640736f, -0.355205f),  FfxFloat32x2(-0.725411f, -0.688316f), FfxFloat32x2(-0.185095f, 0.722648f),   FfxFloat32x2(0.770596f, 0.637324f),
    FfxFloat32x2(-0.921445f, 0.196997f),  FfxFloat32x2(0.076571f, -0.98822f),   FfxFloat32x2(-0.1348f, -0.0908536f),   FfxFloat32x2(0.320109f, 0.257241f),
    FfxFloat32x2(0.994021f, 0.109193f),   FfxFloat32x2(0.304934f, 0.952374f),   FfxFloat32x2(-0.698577f, 0.715535f),   FfxFloat32x2(0.548701f, -0.836019f),
    FfxFloat32x2(-0.443159f, 0.296121f),  FfxFloat32x2(0.15067f, -0.489731f),   FfxFloat32x2(-0.623829f, -0.208167f),  FfxFloat32x2(-0.294778f, -0.596545f),
    FfxFloat32x2(0.334086f, -0.128208f),  FfxFloat32x2(-0.0619831f, 0.311747f), FfxFloat32x2(0.166112f, 0.61626f),     FfxFloat32x2(-0.289127f, -0.957291f),
    FfxFloat32x2(-0.98748f, -0.157745f),  FfxFloat32x2(0.637501f, 0.0651571f),  FfxFloat32x2(0.971376f, -0.237545f),   FfxFloat32x2(-0.0170599f, 0.98059f),
    FfxFloat32x2(-0.442564f, 0.896737f),  FfxFloat32x2(0.48619f, 0.518723f),    FfxFloat32x2(-0.725272f, 0.419965f),   FfxFloat32x2(0.781417f, -0.624009f),
    FfxFloat32x2(-0.899227f, -0.437482f), FfxFloat32x2(0.769219f, 0.33372f),    FfxFloat32x2(-0.414411f, 0.00375378f), FfxFloat32x2(0.262856f, -0.759514f),
};

#endif // FFX_CLASSIFIER_SHADOWS_CALLBACKS_HLSL
