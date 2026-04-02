#ifndef SHADER_INTEROP_H
#define SHADER_INTEROP_H

#if defined(__cplusplus)
#include "Math/Float2x2.h"
#include "Math/Float3x3.h"
#include "Math/Float4x4.h"

using float2x2 = Gleam::Float2x2;
using float3x3 = Gleam::Float3x3;
using float4x4 = Gleam::Float4x4;
using float2 = Gleam::Float2;
using float3 = Gleam::Float3;
using float4 = Gleam::Float4;
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

namespace Gleam {

enum class DispatchRayType
{
	Shading = 0,
	Shadow = 1,
	COUNT
};
#define MAX_RAY_RECURSION_DEPTH 10

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
		Texture2D<T> texture = ResourceDescriptorHeap[SRVIndex(index)];
		return texture.Load(pos);
	}

	T Sample(SamplerState sampler, float2 uv)
	{
		Texture2D<T> texture = ResourceDescriptorHeap[SRVIndex(index)];
	#ifdef SHADER_TARGET_PIXEL
		return texture.Sample(sampler, uv);
	#else
		return texture.SampleLevel(sampler, uv, 0.0);
	#endif
	}

	T SampleLevel(SamplerState sampler, float2 uv, float mip)
	{
		Texture2D<T> texture = ResourceDescriptorHeap[SRVIndex(index)];
		return texture.SampleLevel(sampler, uv, mip);
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