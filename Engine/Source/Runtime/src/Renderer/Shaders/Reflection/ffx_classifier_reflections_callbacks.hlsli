// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef FFX_CLASSIFIER_REFLECTIONS_CALLBACKS_HLSL
#define FFX_CLASSIFIER_REFLECTIONS_CALLBACKS_HLSL

#include "Common.hlsli"
#include "BRDF.hlsli"
#include "ShaderTypes.h"
#include "FidelityFXCore.hlsli"
#include "ReflectionRayList.hlsli"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);
PUSH_CONSTANT(Gleam::ReflectionClassificationConstants, constants);

FfxFloat32Mat4 InvViewProjection()
{
    return camera.invViewProjectionMatrix;
}

FfxFloat32Mat4 Projection()
{
    return camera.projectionMatrix;
}

FfxFloat32Mat4 InvProjection()
{
    return camera.invProjectionMatrix;
}

FfxFloat32Mat4 ViewMatrix()
{
    return camera.viewMatrix;
}

FfxFloat32Mat4 InvView()
{
    return camera.invViewMatrix;
}

FfxFloat32Mat4 PrevViewProjection()
{
    return camera.prevViewProjectionMatrix;
}

FfxUInt32x2 RenderSize()
{
    return FfxUInt32x2(camera.resolution);
}

FfxFloat32x2 InverseRenderSize()
{
    return FfxFloat32x2(1.0f / camera.resolution.x, 1.0f / camera.resolution.y);
}

FfxUInt32 ReflectionWidth()
{
    return FfxUInt32(camera.resolution.x);
}

FfxUInt32 ReflectionHeight()
{
    return FfxUInt32(camera.resolution.y);
}

FfxFloat32 IBLFactor()
{
    return 1.0;
}

FfxFloat32 RoughnessThreshold()
{
    return constants.roughnessThreshold;
}

FfxFloat32 RTRoughnessThreshold()
{
    return constants.roughnessThreshold;
}

FfxFloat32 ReflectionsBackfacingThreshold()
{
    return 1.0;
}

FfxFloat32 VRTVarianceThreshold()
{
    return constants.varianceThreshold;
}

FfxUInt32 SamplesPerQuad()
{
    return constants.samplesPerQuad;
}

FfxBoolean TemporalVarianceGuidedTracingEnabled()
{
    return constants.temporalVarianceGuidedTracing != 0u;
}

FfxUInt32 FrameIndex()
{
    return constants.frameIndex;
}

FfxFloat32 HybridMissWeight()
{
    return 0.0f;
}

FfxFloat32 HybridSpawnRate()
{
    return 0.0f;
}

FfxFloat32x3 LoadWorldSpaceNormal(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[constants.normalTexture];
    return normalize(OctDecode(tex.Load(FfxInt32x3(pixel_coordinate, 0))));
}

FfxFloat32 GetInputDepth(FfxUInt32x2 coordinate)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.depthTexture];
    return tex.Load(FfxInt32x3(FfxInt32x2(coordinate), 0));
}

FfxFloat32 LoadRoughnessFromMaterialParametersInput(FfxUInt32x3 coordinate)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.roughnessTexture];
    FfxFloat32 perceptualRoughness = tex.Load(FfxInt32x3(FfxInt32x2(coordinate.xy), 0));
    return perceptualRoughness * perceptualRoughness;
}

FfxFloat32x2 LoadMotionVector(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[constants.motionVectorTexture];
    return -tex.Load(FfxInt32x3(pixel_coordinate, 0)) * InverseRenderSize();
}

FfxFloat32 SampleVarianceHistory(FfxFloat32x2 coordinate)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.varianceHistoryTexture];
    return tex.SampleLevel(Sampler_Bilinear_Clamp, coordinate, 0.0f).x;
}

FfxFloat32x3 SampleEnvironmentMap(FfxFloat32x3 direction, FfxFloat32 preceptualRoughness)
{
    TextureCube<float4> specularReflection = ResourceDescriptorHeap[constants.specularReflectionTexture];
    FfxFloat32 mipLevel = PerceptualRoughnessToMipLevel(preceptualRoughness, SPECULAR_RADIANCE_MAX_MIP_COUNT - 1);
    return specularReflection.SampleLevel(Sampler_Trilinear_Repeat, direction, mipLevel).rgb;
}

void StoreRadiance(FfxUInt32x2 coordinate, FfxFloat32x4 radiance)
{
    RWTexture2D<float4> tex = ResourceDescriptorHeap[constants.radianceTexture];
    tex[coordinate] = radiance;
}

void IncrementRayCounterHW(FfxUInt32 value, FFX_PARAMETER_OUT FfxUInt32 original_value)
{
    RWByteAddressBuffer buffer = ResourceDescriptorHeap[constants.rayCounterBuffer];
    buffer.InterlockedAdd(REFLECTION_RAY_COUNTER_HW * sizeof(uint), value, original_value);
}

void IncrementDenoiserTileCounter(FFX_PARAMETER_OUT FfxUInt32 original_value)
{
    RWByteAddressBuffer buffer = ResourceDescriptorHeap[constants.rayCounterBuffer];
    buffer.InterlockedAdd(REFLECTION_RAY_COUNTER_DENOISE * sizeof(uint), 1u, original_value);
}

void StoreRayHW(FfxInt32 index, FfxUInt32x2 ray_coord, FfxBoolean copy_horizontal, FfxBoolean copy_vertical, FfxBoolean copy_diagonal)
{
    RWByteAddressBuffer buffer = ResourceDescriptorHeap[constants.rayListBuffer];
    buffer.Store<uint>(index * sizeof(uint), PackReflectionRayCoords(ray_coord, copy_horizontal, copy_vertical, copy_diagonal));
}

void StoreDenoiserTile(FfxInt32 index, FfxUInt32x2 tile_coord)
{
    RWByteAddressBuffer buffer = ResourceDescriptorHeap[constants.denoiserTileListBuffer];
    buffer.Store<uint>(index * sizeof(uint), ((tile_coord.y & 0xffffu) << 16) | ((tile_coord.x & 0xffffu) << 0));
}

void IncrementRayCounterSW(FfxUInt32 value, FFX_PARAMETER_OUT FfxUInt32 original_value)
{
    original_value = 0u;
}

void StoreRay(FfxInt32 index, FfxUInt32x2 ray_coord, FfxBoolean copy_horizontal, FfxBoolean copy_vertical, FfxBoolean copy_diagonal)
{
}

void StoreRaySWHelper(FfxInt32 index)
{
}

FfxUInt32 LoadHitCounterHistory(FfxUInt32x2 coordinate)
{
    return 0u;
}

void StoreHitCounter(FfxUInt32x2 coordinate, FfxUInt32 value)
{
}

#endif // FFX_CLASSIFIER_REFLECTIONS_CALLBACKS_HLSL
