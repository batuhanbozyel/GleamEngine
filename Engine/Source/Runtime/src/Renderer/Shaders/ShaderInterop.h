#pragma once

#if defined(__cplusplus)
using float2x2 = Gleam::Matrix2;
using float3x3 = Gleam::Matrix3;
using float4x4 = Gleam::Matrix4;
using float2 = Gleam::Vector2;
using float3 = Gleam::Vector3;
using float4 = Gleam::Vector4;
#endif

#ifndef __spirv__
#define PUSH_CONSTANT_SIZE 128
#define PUSH_CONSTANT_SLOT 9
#define PUSH_CONSTANT_REGISTER 999
#endif

namespace Gleam {

#define CBV_SRV_HEAP_SIZE (128 * 1024)
#define InvalidResourceIndex ShaderResourceIndex(-1)
#define SRVIndex(index) (index)

#if defined(USE_DIRECTX_RENDERER)
#define UAVIndex(index) (index + CBV_SRV_HEAP_SIZE)
#else
#define UAVIndex(index) SRVIndex(index)
#endif

#ifdef __cplusplus
struct ShaderResourceIndex
{
    uint32_t data;

    ShaderResourceIndex()
        : data(InvalidResourceIndex.data)
    {
    }

    explicit ShaderResourceIndex(uint32_t index)
        : data(index)
    {
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
#else
typedef uint ShaderResourceIndex;
#endif

struct ConstantBufferView
{
	ShaderResourceIndex index;

#ifdef __HLSL_VERSION
	template<typename T>
	T Load()
	{
		ByteAddressBuffer buffer = ResourceDescriptorHeap[SRVIndex(index)];
		return buffer.Load<T>(0);
	}
#else
	ConstantBufferView& operator=(ShaderResourceIndex other)
	{
		index = other;
		return *this;
	}
#endif
};

struct BufferResourceView
{
	ShaderResourceIndex index;

#ifdef __HLSL_VERSION
	template<typename T>
	T Load(uint id)
	{
		ByteAddressBuffer buffer = ResourceDescriptorHeap[SRVIndex(index)];
		return buffer.Load<T>(sizeof(T) * id);
	}
#else
	BufferResourceView& operator=(ShaderResourceIndex other)
	{
		index = other;
		return *this;
	}
#endif
};

template<typename T>
struct Texture2DResourceView
{
	ShaderResourceIndex index;

#ifdef __HLSL_VERSION
	T Load(uint2 pos)
	{
		Texture2D<T> texture = ResourceDescriptorHeap[SRVIndex(index)];
		return texture.Load(uint3(pos, 0));
	}

	T Sample(SamplerState sampler, float2 uv, float mip = 0.0f)
	{
		Texture2D<T> texture = ResourceDescriptorHeap[SRVIndex(index)];
		return texture.Sample(sampler, uv, mip);
	}
#else
	Texture2DResourceView& operator=(ShaderResourceIndex other)
	{
		index = other;
		return *this;
	}
#endif
};

template<typename T>
struct Texture3DResourceView
{
	ShaderResourceIndex index;

#ifdef __HLSL_VERSION
	T Load(uint3 pos)
	{
		Texture3D<T> texture = ResourceDescriptorHeap[SRVIndex(index)];
		return texture.Load(uint4(pos, 0));
	}

	T Sample(SamplerState sampler, float3 uv, float mip = 0.0f)
	{
		Texture3D<T> texture = ResourceDescriptorHeap[SRVIndex(index)];
		return texture.Sample(sampler, uv, mip);
	}
#else
	Texture3DResourceView& operator=(ShaderResourceIndex other)
	{
		index = other;
		return *this;
	}
#endif
};

} // namespace Gleam
