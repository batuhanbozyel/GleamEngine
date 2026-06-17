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

#ifndef FFX_DENOISER_SHADOWS_CALLBACKS_HLSL
#define FFX_DENOISER_SHADOWS_CALLBACKS_HLSL

#include "Common.hlsli"
#include "ShaderTypes.h"
#include "FidelityFXCore.hlsli"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);

#if defined(FFX_DNSR_SHADOWS_FILTER_PASS)
PUSH_CONSTANT(Gleam::ShadowDenoiserFilterConstants, constants);
#elif defined(FFX_DNSR_SHADOWS_TILECLASSIFICATION_PASS)
PUSH_CONSTANT(Gleam::ShadowDenoiserTileClassificationConstants, constants);
#endif

FfxUInt32 LaneIdToBitShift(FfxUInt32x2 localID)
{
    return localID.y * SHADOW_TILE_WIDTH + localID.x;
}

FfxBoolean WaveMaskToBool(FfxUInt32 mask, FfxUInt32x2 localID)
{
    return bool((1u << LaneIdToBitShift(localID)) & mask);
}

FfxInt32x2 BufferDimensions()
{
    return FfxInt32x2(int2(camera.resolution));
}

FfxFloat32x2 InvBufferDimensions()
{
    return FfxFloat32x2(1.0f / camera.resolution.x, 1.0f / camera.resolution.y);
}

FfxFloat32Mat4 ProjectionInverse()
{
    return camera.invProjectionMatrix;
}

FfxFloat32Mat4 ViewProjectionInverse() 
{
    return camera.invViewProjectionMatrix;
}

FfxFloat32x3 Eye()
{
    return camera.position;
}

FfxFloat32 LoadDepth(FfxInt32x2 p)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.depth];
    return tex.Load(int3(p, 0));
}

FfxBoolean IsShadowReciever(FfxUInt32x2 p)
{
    FfxFloat32 depth = LoadDepth(p);
    return (depth > 0.0f) && (depth < 1.0f);
}

FfxFloat32x3 LoadNormals(FfxInt32x2 p)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[constants.normalTexture];
    return OctDecode(tex.Load(int3(p, 0)));
}

#if defined(FFX_DNSR_SHADOWS_FILTER_PASS)
FfxFloat32 DepthSimilaritySigma()
{
    return 1.0f;
}

#if FFX_HALF
FfxFloat16x2 LoadFilterInput(FfxUInt32x2 p)
{
    Texture2D<float16_t2> tex = ResourceDescriptorHeap[constants.filterInput];
    return (FfxFloat16x2)tex.Load(int3(p, 0));
}
#endif

FfxUInt32 LoadTileMetaData(FfxUInt32 p)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[constants.tileMetadata];
    return buffer.Load<uint>(p * sizeof(uint));
}

void StoreHistory(FfxUInt32x2 p, FfxFloat32x2 val)
{
    RWTexture2D<float2> tex = ResourceDescriptorHeap[constants.history];
    tex[p] = val;
}

void StoreFilterOutput(FfxUInt32x2 p, FfxFloat32 val)
{
    RWTexture2D<unorm float> tex = ResourceDescriptorHeap[constants.shadowMaskOutput];
    tex[p] = val;
}
#endif // FFX_DNSR_SHADOWS_FILTER_PASS

#if defined(FFX_DNSR_SHADOWS_TILECLASSIFICATION_PASS)
FfxInt32 IsFirstFrame()
{
    return constants.isFirstFrame;
}

FfxFloat32Mat4 ReprojectionMatrix()
{
    return constants.reprojectionMatrix;
}

FfxFloat32 LoadPreviousDepth(FfxInt32x2 p)
{
    Texture2D<float> tex = ResourceDescriptorHeap[constants.previousDepth];
    return tex.Load(int3(p, 0));
}

FfxFloat32x2 LoadVelocity(FfxInt32x2 p)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[constants.velocity];
    return tex.Load(int3(p, 0)) * InvBufferDimensions();
}

FfxFloat32 LoadHistory(FfxFloat32x2 uv)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[constants.historyShadow];
    return tex.SampleLevel(Sampler_Bilinear_Clamp, uv, 0).x;
}

FfxFloat32x3 LoadPreviousMomentsBuffer(FfxInt32x2 p)
{
    Texture2D<float3> tex = ResourceDescriptorHeap[constants.previousMoments];
    return tex.Load(int3(p, 0));
}

FfxUInt32 LoadRaytracedShadowMask(FfxUInt32 linearIndex)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[constants.hitMaskResults];
    return buffer.Load<uint>(linearIndex * sizeof(uint));
}

void StoreMetadata(FfxUInt32 p, FfxUInt32 val)
{
    RWByteAddressBuffer buffer = ResourceDescriptorHeap[constants.tileMetadata];
    buffer.Store<uint>(p * sizeof(uint), val);
}

void StoreMoments(FfxUInt32x2 p, FfxFloat32x3 val)
{
    RWTexture2D<float3> tex = ResourceDescriptorHeap[constants.currentMoments];
    tex[p] = val;
}

void StoreReprojectionResults(FfxUInt32x2 p, FfxFloat32x2 val)
{
    RWTexture2D<float2> tex = ResourceDescriptorHeap[constants.reprojectionResults];
    tex[p] = val;
}
#endif // FFX_DNSR_SHADOWS_TILECLASSIFICATION_PASS

#endif // FFX_DENOISER_SHADOWS_CALLBACKS_HLSL
