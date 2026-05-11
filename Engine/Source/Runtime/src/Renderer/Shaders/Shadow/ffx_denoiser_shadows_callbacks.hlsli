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

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);

// ----------------------------------------------------------------
// FidelityFX type aliases (replaces ffx_core.h)
// ----------------------------------------------------------------
#define FFX_GPU  1
#define FFX_HALF 1

typedef float      FfxFloat32;
typedef float2     FfxFloat32x2;
typedef float3     FfxFloat32x3;
typedef float4     FfxFloat32x4;
typedef float4x4   FfxFloat32Mat4;
typedef int        FfxInt32;
typedef int2       FfxInt32x2;
typedef int3       FfxInt32x3;
typedef uint       FfxUInt32;
typedef uint2      FfxUInt32x2;
typedef uint3      FfxUInt32x3;
typedef bool       FfxBoolean;

#if FFX_HALF
typedef float16_t    FfxFloat16;
typedef float16_t2   FfxFloat16x2;
typedef float16_t3   FfxFloat16x3;
#endif

#define FFX_TRUE  true
#define FFX_FALSE false
#define FFX_GROUPSHARED          groupshared
#define FFX_GROUP_MEMORY_BARRIER GroupMemoryBarrierWithGroupSync()
#define FFX_MATRIX_MULTIPLY(a,b) mul(a,b)

#define ffxWaveLaneCount()   WaveGetLaneCount()
#define ffxWaveAllTrue(x)    WaveActiveAllTrue(x)
#define ffxQuadReadX(x)      QuadReadAcrossX(x)
#define ffxQuadReadY(x)      QuadReadAcrossY(x)
#define ffxLerp(a,b,t)       lerp(a,b,t)
#define ffxPow(x,y)          pow(abs(x),y)
#define ffxSaturate(x)       saturate(x)
#define ffxReciprocal(x)     rcp(x)
#define ffxPackHalf2x16(v)   (f32tof16((v).x) | (f32tof16((v).y) << 16))
#define ffxUnpackF16(v)      float16_t2(f16tof32((v) & 0xFFFF), f16tof32(((v) >> 16) & 0xFFFF))
#define FFX_GREATER_THAN(a,b) ((a) > (b))
#define FFX_LESS_THAN(a,b)    ((a) < (b))
#define FFX_EQUAL(a,b)        ((a) == (b))

FfxUInt32 ffxBitfieldExtract(FfxUInt32 src, FfxUInt32 off, FfxUInt32 bits)
{
    FfxUInt32 mask = (1u << bits) - 1;
    return (src >> off) & mask;
}

FfxUInt32 ffxBitfieldInsert(FfxUInt32 src, FfxUInt32 ins, FfxUInt32 mask)
{
    return (ins & mask) | (src & (~mask));
}

FfxUInt32 ffxBitfieldInsertMask(FfxUInt32 src, FfxUInt32 ins, FfxUInt32 bits)
{
    FfxUInt32 mask = (1u << bits) - 1;
    return (ins & mask) | (src & (~mask));
}

/// A helper function performing a remap 64x1 to 8x8 remapping which is necessary for 2D wave reductions.
///
/// The 64-wide lane indices to 8x8 remapping is performed as follows:
/// 
///     00 01 08 09 10 11 18 19
///     02 03 0a 0b 12 13 1a 1b
///     04 05 0c 0d 14 15 1c 1d
///     06 07 0e 0f 16 17 1e 1f
///     20 21 28 29 30 31 38 39
///     22 23 2a 2b 32 33 3a 3b
///     24 25 2c 2d 34 35 3c 3d
///     26 27 2e 2f 36 37 3e 3f
///
/// @param [in] a       The input 1D coordinate to remap.
/// 
/// @returns
/// The remapped 2D coordinates.
/// 
/// @ingroup GPUCore
FfxUInt32x2 ffxRemapForWaveReduction(FfxUInt32 a)
{
    return FfxUInt32x2(ffxBitfieldInsertMask(ffxBitfieldExtract(a, 2u, 3u), a, 1u), ffxBitfieldInsertMask(ffxBitfieldExtract(a, 3u, 3u), ffxBitfieldExtract(a, 1u, 2u), 2u));
}

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

FfxFloat32Mat4 ReprojectionMatrix()
{
    return camera.prevViewProjectionMatrix;
}

FfxFloat32x3 Eye()
{
    return camera.position;
}

FfxFloat32 LoadDepth(FfxInt32x2 p)
{
    Texture2D<float> tex = ResourceDescriptorHeap[dnsr.depth];
    return tex.Load(int3(p, 0));
}

FfxBoolean IsShadowReciever(FfxUInt32x2 p)
{
    FfxFloat32 depth = LoadDepth(p);
    return (depth > 0.0f) && (depth < 1.0f);
}

FfxFloat32x3 LoadNormals(FfxInt32x2 p)
{
    int2 dims    = BufferDimensions();
    float2 invD  = InvBufferDimensions();

    int2 p1x = clamp(p + int2(1, 0), int2(0, 0), dims - 1);
    int2 p1y = clamp(p + int2(0, 1), int2(0, 0), dims - 1);

    float d0  = LoadDepth(p);
    float d1x = LoadDepth(p1x);
    float d1y = LoadDepth(p1y);

    // Reconstruct view-space positions from NDC depth (left-handed, Y-up).
    float2 uv0  = (p   + 0.5f) * invD;
    float2 uv1x = (p1x + 0.5f) * invD;
    float2 uv1y = (p1y + 0.5f) * invD;

    float4 c0  = mul(camera.invProjectionMatrix, float4(uv0.x  * 2.0f - 1.0f, 1.0f - uv0.y  * 2.0f, d0,  1.0f));
    float4 c1x = mul(camera.invProjectionMatrix, float4(uv1x.x * 2.0f - 1.0f, 1.0f - uv1x.y * 2.0f, d1x, 1.0f));
    float4 c1y = mul(camera.invProjectionMatrix, float4(uv1y.x * 2.0f - 1.0f, 1.0f - uv1y.y * 2.0f, d1y, 1.0f));

    float3 v0  = c0.xyz  / c0.w;
    float3 v1x = c1x.xyz / c1x.w;
    float3 v1y = c1y.xyz / c1y.w;

    float3 normal = normalize(cross(v1x - v0, v1y - v0));
    return normalize(mul((float3x3)camera.invViewMatrix, normal));
}

#if defined(FFX_DNSR_SHADOWS_FILTER_PASS)
PUSH_CONSTANT(Gleam::ShadowDenoiserFilterConstants, dnsr);

FfxFloat32 DepthSimilaritySigma()
{
    return 1.0f;
}

#if FFX_HALF
FfxFloat16x2 LoadFilterInput(FfxUInt32x2 p)
{
    Texture2D<float16_t2> tex = ResourceDescriptorHeap[dnsr.filterInput];
    return (FfxFloat16x2)tex.Load(int3(p, 0));
}
#endif

FfxUInt32 LoadTileMetaData(FfxUInt32 p)
{
    Texture2D<uint> tex = ResourceDescriptorHeap[dnsr.tileMetadata];
    return tex[p];
}

void StoreHistory(FfxUInt32x2 p, FfxFloat32x2 val)
{
    RWTexture2D<float2> tex = ResourceDescriptorHeap[dnsr.history];
    tex[p] = val;
}

void StoreFilterOutput(FfxUInt32x2 p, FfxFloat32 val)
{
    RWTexture2D<unorm float> tex = ResourceDescriptorHeap[dnsr.shadowMaskOutput];
    tex[p] = val;
}
#endif // FFX_DNSR_SHADOWS_FILTER_PASS

#if defined(FFX_DNSR_SHADOWS_TILECLASSIFICATION_PASS)
PUSH_CONSTANT(Gleam::ShadowDenoiserTileClassificationConstants, dnsr);

FfxInt32 IsFirstFrame()
{
    return dnsr.isFirstFrame;
}

FfxFloat32 LoadPreviousDepth(FfxInt32x2 p)
{
    Texture2D<float> tex = ResourceDescriptorHeap[dnsr.previousDepth];
    return tex.Load(int3(p, 0));
}

FfxFloat32x2 LoadVelocity(FfxInt32x2 p)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[dnsr.velocity];
    return tex.Load(int3(p, 0));
}

FfxFloat32 LoadHistory(FfxFloat32x2 uv)
{
    Texture2D<float2> tex = ResourceDescriptorHeap[dnsr.historyShadow];
    return tex.SampleLevel(Sampler_Bilinear_Clamp, uv, 0).x;
}

FfxFloat32x3 LoadPreviousMomentsBuffer(FfxInt32x2 p)
{
    Texture2D<float4> tex = ResourceDescriptorHeap[dnsr.previousMoments];
    return tex.Load(int3(p, 0)).xyz;
}

FfxUInt32 LoadRaytracedShadowMask(FfxUInt32 linearIndex)
{
    uint tilesWide = (uint(BufferDimensions().x) + (SHADOW_TILE_WIDTH - 1u)) / SHADOW_TILE_WIDTH;
    uint2 tileCoord = uint2(linearIndex % tilesWide, linearIndex / tilesWide);
    Texture2D<uint> tex = ResourceDescriptorHeap[dnsr.hitMaskResults];
    return tex.Load(int3(tileCoord, 0));
}

void StoreMetadata(FfxUInt32 p, FfxUInt32 val)
{
    RWTexture2D<uint> tex = ResourceDescriptorHeap[dnsr.tileMetadata];
    tex[p] = val;
}

void StoreMoments(FfxUInt32x2 p, FfxFloat32x3 val)
{
    RWTexture2D<float3> tex = ResourceDescriptorHeap[dnsr.currentMoments];
    tex[p] = val;
}

void StoreReprojectionResults(FfxUInt32x2 p, FfxFloat32x2 val)
{
    RWTexture2D<float2> tex = ResourceDescriptorHeap[dnsr.reprojectionResults];
    tex[p] = val;
}
#endif // FFX_DNSR_SHADOWS_TILECLASSIFICATION_PASS

#endif // FFX_DENOISER_SHADOWS_CALLBACKS_HLSL
