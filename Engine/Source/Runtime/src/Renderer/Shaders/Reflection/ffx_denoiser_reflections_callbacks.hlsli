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

#ifndef FFX_DENOISER_REFLECTIONS_CALLBACKS_HLSL
#define FFX_DENOISER_REFLECTIONS_CALLBACKS_HLSL

#include "Common.hlsli"
#include "ShaderTypes.h"
#include "FidelityFXCore.hlsli"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);

#if defined(FFX_DNSR_REFLECTIONS_REPROJECT_PASS)
PUSH_CONSTANT(Gleam::ReflectionDenoiserReprojectConstants, constants);
#elif defined(FFX_DNSR_REFLECTIONS_PREFILTER_PASS)
PUSH_CONSTANT(Gleam::ReflectionDenoiserPrefilterConstants, constants);
#elif defined(FFX_DNSR_REFLECTIONS_RESOLVE_TEMPORAL_PASS)
PUSH_CONSTANT(Gleam::ReflectionDenoiserResolveTemporalConstants, constants);
#endif

FfxFloat32Mat4 InvProjection()
{
    return camera.invProjectionMatrix;
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

FfxFloat32 RoughnessThreshold()
{
    return constants.roughnessThreshold;
}

FfxUInt32 GetDenoiserTile(FfxUInt32 group_id)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[constants.tileListBuffer];
    return buffer.Load<uint>(group_id * sizeof(uint));
}

FfxFloat16 FFX_DNSR_Reflections_LoadRoughness(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.roughnessTexture];
    FfxFloat16 perceptualRoughness = (FfxFloat16)tex.Load(FfxInt32x3(pixel_coordinate, 0));
    return perceptualRoughness * perceptualRoughness;
}

FfxFloat16x3 LoadRadianceH(FfxInt32x3 coordinate)
{
    Texture2D<float4> tex = ResourceDescriptorHeap[constants.radianceTexture];
    return (FfxFloat16x3)tex.Load(coordinate).xyz;
}

FfxFloat16 LoadVarianceH(FfxInt32x3 coordinate)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.varianceTexture];
    return (FfxFloat16)tex.Load(coordinate).x;
}

FfxFloat16x3 FFX_DNSR_Reflections_LoadRadiance(FfxInt32x2 pixel_coordinate)
{
    return LoadRadianceH(FfxInt32x3(pixel_coordinate, 0));
}

FfxFloat16 FFX_DNSR_Reflections_LoadVariance(FfxInt32x2 pixel_coordinate)
{
    return LoadVarianceH(FfxInt32x3(pixel_coordinate, 0));
}

void FFX_DNSR_Reflections_StoreVariance(FfxInt32x2 pixel_coordinate, FfxFloat16 value)
{
    RWTexture2D<float> tex = ResourceDescriptorHeap[constants.varianceOutputTexture];
    tex[pixel_coordinate] = value;
}

#if defined(FFX_DNSR_REFLECTIONS_REPROJECT_PASS) || defined(FFX_DNSR_REFLECTIONS_PREFILTER_PASS)
FfxFloat32 FFX_DENOISER_LoadDepth(FfxInt32x2 pixel_coordinate, FfxInt32 mip)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.depthTexture];
    return tex.Load(FfxInt32x3(pixel_coordinate, 0));
}

FfxFloat32 FFX_DNSR_Reflections_LoadDepth(FfxInt32x2 pixel_coordinate)
{
    return FFX_DENOISER_LoadDepth(pixel_coordinate, 0);
}

FfxFloat16x3 FFX_DENOISER_LoadWorldSpaceNormalH(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[constants.normalTexture];
    return (FfxFloat16x3)normalize(OctDecode(tex.Load(FfxInt32x3(pixel_coordinate, 0))));
}

FfxFloat16x3 FFX_DNSR_Reflections_LoadWorldSpaceNormal(FfxInt32x2 pixel_coordinate)
{
    return FFX_DENOISER_LoadWorldSpaceNormalH(pixel_coordinate);
}
#endif // FFX_DNSR_REFLECTIONS_REPROJECT_PASS || FFX_DNSR_REFLECTIONS_PREFILTER_PASS

#if defined(FFX_DNSR_REFLECTIONS_PREFILTER_PASS) || defined(FFX_DNSR_REFLECTIONS_RESOLVE_TEMPORAL_PASS)
FfxFloat16x3 FFX_DNSR_Reflections_SampleAverageRadiance(FfxFloat32x2 uv)
{
    Texture2D<float3> tex = ResourceDescriptorHeap[constants.averageRadianceTexture];
    return (FfxFloat16x3)tex.SampleLevel(Sampler_Bilinear_Clamp, uv, 0.0f);
}

void FFX_DNSR_Reflections_StoreDenoisedRadiance(FfxInt32x2 pixel_coordinate, FfxFloat16x3 radiance, FfxFloat16 variance)
{
    RWTexture2D<float4> tex = ResourceDescriptorHeap[constants.radianceOutputTexture];
    tex[pixel_coordinate] = FfxFloat32x4(radiance, 0.0f);
    FFX_DNSR_Reflections_StoreVariance(pixel_coordinate, variance);
}
#endif // FFX_DNSR_REFLECTIONS_PREFILTER_PASS || FFX_DNSR_REFLECTIONS_RESOLVE_TEMPORAL_PASS

#if defined(FFX_DNSR_REFLECTIONS_REPROJECT_PASS)
FfxFloat32 TemporalStabilityFactor()
{
    return constants.temporalStabilityFactor;
}

FfxFloat32x2 FFX_DNSR_Reflections_LoadMotionVector(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[constants.motionVectorTexture];
    return tex.Load(FfxInt32x3(pixel_coordinate, 0)) * InverseRenderSize();
}

FfxFloat16 FFX_DNSR_Reflections_LoadRayLength(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float4> tex = ResourceDescriptorHeap[constants.radianceTexture];
    return (FfxFloat16)tex.Load(FfxInt32x3(pixel_coordinate, 0)).w;
}

FfxFloat16x3 FFX_DNSR_Reflections_LoadRadianceHistory(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float4> tex = ResourceDescriptorHeap[constants.radianceHistoryTexture];
    return (FfxFloat16x3)tex.Load(FfxInt32x3(pixel_coordinate, 0)).xyz;
}

FfxFloat16x3 FFX_DNSR_Reflections_SampleRadianceHistory(FfxFloat32x2 uv)
{
    Texture2D<float4> tex = ResourceDescriptorHeap[constants.radianceHistoryTexture];
    return (FfxFloat16x3)tex.SampleLevel(Sampler_Bilinear_Clamp, uv, 0.0f).xyz;
}

FfxFloat16 FFX_DNSR_Reflections_SampleVarianceHistory(FfxFloat32x2 uv)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.varianceTexture];
    return (FfxFloat16)tex.SampleLevel(Sampler_Bilinear_Clamp, uv, 0.0f);
}

FfxFloat16 FFX_DNSR_Reflections_SampleNumSamplesHistory(FfxFloat32x2 uv)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.sampleCountTexture];
    return (FfxFloat16)tex.SampleLevel(Sampler_Bilinear_Clamp, uv, 0.0f);
}

FfxFloat32 FFX_DNSR_Reflections_LoadDepthHistory(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.previousDepthTexture];
    return tex.Load(FfxInt32x3(pixel_coordinate, 0));
}

FfxFloat32 FFX_DNSR_Reflections_SampleDepthHistory(FfxFloat32x2 uv)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.previousDepthTexture];
    return tex.SampleLevel(Sampler_Bilinear_Clamp, uv, 0.0f);
}

FfxFloat16x3 FFX_DNSR_Reflections_LoadWorldSpaceNormalHistory(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[constants.previousNormalTexture];
    return (FfxFloat16x3)normalize(OctDecode(tex.Load(FfxInt32x3(pixel_coordinate, 0))));
}

FfxFloat16x3 FFX_DNSR_Reflections_SampleWorldSpaceNormalHistory(FfxFloat32x2 uv)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[constants.previousNormalTexture];
    return (FfxFloat16x3)normalize(OctDecode(tex.SampleLevel(Sampler_Bilinear_Clamp, uv, 0.0f)));
}

FfxFloat16 FFX_DNSR_Reflections_SampleRoughnessHistory(FfxFloat32x2 uv)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.previousRoughnessTexture];
    FfxFloat16 perceptualRoughness = (FfxFloat16)tex.SampleLevel(Sampler_Bilinear_Clamp, uv, 0.0f);
    return perceptualRoughness * perceptualRoughness;
}

void FFX_DNSR_Reflections_StoreNumSamples(FfxInt32x2 pixel_coordinate, FfxFloat16 value)
{
    RWTexture2D<float> tex = ResourceDescriptorHeap[constants.sampleCountOutputTexture];
    tex[pixel_coordinate] = value;
}

void FFX_DNSR_Reflections_StoreRadianceReprojected(FfxInt32x2 pixel_coordinate, FfxFloat16x3 value)
{
    RWTexture2D<float4> tex = ResourceDescriptorHeap[constants.reprojectedRadianceTexture];
    tex[pixel_coordinate] = FfxFloat32x4(value, 0.0f);
}

void FFX_DNSR_Reflections_StoreAverageRadiance(FfxInt32x2 pixel_coordinate, FfxFloat16x3 value)
{
    RWTexture2D<float3> tex = ResourceDescriptorHeap[constants.averageRadianceOutputTexture];
    tex[pixel_coordinate] = value;
}
#endif // FFX_DNSR_REFLECTIONS_REPROJECT_PASS

#if defined(FFX_DNSR_REFLECTIONS_PREFILTER_PASS)
void FFX_DNSR_Reflections_StorePrefilteredReflections(FfxInt32x2 pixel_coordinate, FfxFloat16x3 radiance, FfxFloat16 variance)
{
    FFX_DNSR_Reflections_StoreDenoisedRadiance(pixel_coordinate, radiance, variance);
}
#endif // FFX_DNSR_REFLECTIONS_PREFILTER_PASS

#if defined(FFX_DNSR_REFLECTIONS_RESOLVE_TEMPORAL_PASS)
FfxFloat32 TemporalStabilityFactor()
{
    return constants.temporalStabilityFactor;
}

FfxFloat16 FFX_DNSR_Reflections_LoadNumSamples(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.sampleCountTexture];
    return (FfxFloat16)tex.Load(FfxInt32x3(pixel_coordinate, 0));
}

FfxFloat16x3 FFX_DNSR_Reflections_LoadRadianceReprojected(FfxInt32x2 pixel_coordinate)
{
    Texture2D<float4> tex = ResourceDescriptorHeap[constants.reprojectedRadianceTexture];
    return (FfxFloat16x3)tex.Load(FfxInt32x3(pixel_coordinate, 0)).xyz;
}

void FFX_DNSR_Reflections_StoreTemporalAccumulation(FfxInt32x2 pixel_coordinate, FfxFloat16x3 radiance, FfxFloat16 variance)
{
    FFX_DNSR_Reflections_StoreDenoisedRadiance(pixel_coordinate, radiance, variance);
}
#endif // FFX_DNSR_REFLECTIONS_RESOLVE_TEMPORAL_PASS

#endif // FFX_DENOISER_REFLECTIONS_CALLBACKS_HLSL
