#ifndef SHADER_INTEROP_H
#define SHADER_INTEROP_H

#if defined(__cplusplus)
#include "Math/Float2x2.h"
#include "Math/Float3x3.h"
#include "Math/Float4x4.h"
#include "Math/Plane.h"

using float2x2 = Gleam::Float2x2;
using float3x3 = Gleam::Float3x3;
using float4x4 = Gleam::Float4x4;
using float2 = Gleam::Float2;
using float3 = Gleam::Float3;
using float4 = Gleam::Float4;

using int2 = Gleam::Int2;
using int3 = Gleam::Int3;
using int4 = Gleam::Int4;

using uint2 = Gleam::UInt2;
using uint3 = Gleam::UInt3;
using uint4 = Gleam::UInt4;
#endif

#define PUSH_CONSTANT_SIZE 128
#define PUSH_CONSTANT_SLOT 9
#define PUSH_CONSTANT_REGISTER 999

#define CBV_SRV_HEAP_SIZE (128 * 1024)
#define InvalidResourceIndex uint32_t(-1)
#define SRVIndex(index) (index)

#if defined(USE_DIRECTX_RENDERER)
#define UAVIndex(index) (index + CBV_SRV_HEAP_SIZE)
#else
#define UAVIndex(index) SRVIndex(index)
#endif

// Meshlet limits: 64v / 124t is the common cross-platform sweet spot.
// 124 (multiple of 4) packs micro-indices well and fits Apple's 128-thread mesh-threadgroup cap.
#define MAX_MESHLET_VERTICES    64
#define MAX_MESHLET_TRIANGLES   124
#define MESH_AMPLIFICATION_THREADS 32u
#define MESH_SHADER_THREADS        128u

#define VISIBILITY_RESOLVE_GROUP_SIZE 64u

// Visibility buffer bit budget (see VisibilityBufferCommon.hlsli for the full encoding):
// R channel packs (batchIndex << 17) | (instanceID + 1); instanceID + 1 fits 17 bits
// since MaxMeshInstances = 65536, leaving 15 bits for the material batch index.
#define VISIBILITY_TRIANGLE_BITS 7u
#define VISIBILITY_TRIANGLE_MASK 0x7Fu
#define VISIBILITY_INSTANCE_BITS 17u
#define VISIBILITY_INSTANCE_MASK 0x1FFFFu
#define VISIBILITY_MAX_BATCHES (1u << (32u - VISIBILITY_INSTANCE_BITS))

namespace Gleam {

#ifdef __cplusplus
struct ShaderResourceIndex
{
    uint32_t data;

    ShaderResourceIndex()
        : data(InvalidResourceIndex)
    {
    }

    ShaderResourceIndex(uint32_t index)
        : data(SRVIndex(index))
    {
    }

	bool operator==(uint32_t index) const
	{
		return data == index;
	}

	bool operator!=(uint32_t index) const
	{
		return data != index;
	}

    bool operator==(const ShaderResourceIndex& other) const
    {
        return data == other.data;
    }

    bool operator!=(const ShaderResourceIndex& other) const
    {
        return data != other.data;
    }
};

struct UnorderedAccessIndex
{
	uint32_t data;

	UnorderedAccessIndex()
		: data(InvalidResourceIndex)
	{
	}

	UnorderedAccessIndex(uint32_t index)
		: data(UAVIndex(index))
	{
	}

	UnorderedAccessIndex(ShaderResourceIndex index)
		: data(UAVIndex(index.data))
	{
	}

	UnorderedAccessIndex& operator=(uint32_t index)
	{
		data = UAVIndex(index);
		return *this;
	}

	UnorderedAccessIndex& operator=(ShaderResourceIndex index)
	{
		data = UAVIndex(index.data);
		return *this;
	}

	bool operator==(const UnorderedAccessIndex& other) const
	{
		return data == other.data;
	}

	bool operator!=(const UnorderedAccessIndex& other) const
	{
		return data != other.data;
	}
};
static_assert(sizeof(ShaderResourceIndex) == sizeof(uint32_t));
static_assert(sizeof(UnorderedAccessIndex) == sizeof(uint32_t));
#else
typedef uint ShaderResourceIndex;
typedef uint UnorderedAccessIndex;
typedef float4 Plane;
#endif

struct TextureResourceView {};

template<typename T>
struct Texture2DResourceView : TextureResourceView
{
	ShaderResourceIndex index;
	uint32_t padding0;
	uint32_t padding1;
	uint32_t padding2;

#ifdef __HLSL_VERSION
	T Load(uint3 pos)
	{
		Texture2D<T> texture = ResourceDescriptorHeap[index];
		return texture.Load(pos);
	}

	// Pick a better sampling method for real-time raytracing
	// https://media.contentapi.ea.com/content/dam/ea/seed/presentations/2019-ray-tracing-gems-chapter-20-akenine-moller-et-al.pdf
	T Sample(SamplerState sampler, float2 uv)
	{
	#ifdef SHADER_TARGET_PIXEL
		Texture2D<T> texture = ResourceDescriptorHeap[index];
		return texture.Sample(sampler, uv);
	#else
		Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
		return texture.SampleLevel(sampler, uv, 0.0);
	#endif
	}

	T SampleLevel(SamplerState sampler, float2 uv, float mip)
	{
	#ifdef SHADER_TARGET_PIXEL
		Texture2D<T> texture = ResourceDescriptorHeap[index];
	#else
		Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
	#endif
		return texture.SampleLevel(sampler, uv, mip);
	}

	T SampleGrad(SamplerState sampler, float2 uv, float2 ddxUV, float2 ddyUV)
	{
	#ifdef SHADER_TARGET_PIXEL
		Texture2D<T> texture = ResourceDescriptorHeap[index];
	#else
		Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
	#endif
		return texture.SampleGrad(sampler, uv, ddxUV, ddyUV);
	}
#else
    Texture2DResourceView() = default;
    Texture2DResourceView(ShaderResourceIndex index)
        : index(index)
    {
        
    }
#endif

	bool IsValid()
	{
		return index != InvalidResourceIndex;
	}
};

} // namespace Gleam
#endif // SHADER_INTEROP_H
