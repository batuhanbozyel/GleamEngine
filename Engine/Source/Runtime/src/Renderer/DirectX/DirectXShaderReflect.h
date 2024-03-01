#pragma once
#include "Renderer/Shader.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

#include <d3d12shader.h>
#include <dxcapi.h>

namespace Gleam {

struct Shader::Reflection
{
	static inline D3D12_SHADER_INPUT_BIND_DESC invalidResource = { .Type = D3D_SHADER_INPUT_TYPE(UINT_MAX) };

	ID3D12ShaderReflection* reflection;
	TArray<D3D12_SHADER_INPUT_BIND_DESC> SRVs;
	TArray<D3D12_SHADER_INPUT_BIND_DESC> CBVs;
	TArray<D3D12_SHADER_INPUT_BIND_DESC> UAVs;
	TArray<D3D12_SHADER_INPUT_BIND_DESC> samplers;

	Reflection(const DirectXDevice* device, const D3D12_SHADER_BYTECODE* shaderBytecode)
	{
		DxcBuffer reflectionBuffer{};
		reflectionBuffer.Ptr = shaderBytecode->pShaderBytecode;
		reflectionBuffer.Size = shaderBytecode->BytecodeLength;
		reflectionBuffer.Encoding = DXC_CP_ACP;
		device->GetDxcUtils()->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection));

		D3D12_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);

		for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC desc;
			DX_CHECK(reflection->GetResourceBindingDesc(i, &desc));

			switch (desc.Type)
			{
				case D3D_SIT_SAMPLER:
				{
					samplers.push_back(desc);
					break;
				}
				case D3D_SIT_TEXTURE:
				case D3D_SIT_STRUCTURED:
				case D3D_SIT_BYTEADDRESS:
				{
					SRVs.push_back(desc);
					break;
				}
				case D3D_SIT_CBUFFER:
				{
					CBVs.push_back(desc);
					break;
				}
				case D3D_SIT_UAV_RWTYPED:
				case D3D_SIT_UAV_RWSTRUCTURED:
				case D3D_SIT_UAV_RWBYTEADDRESS:
				{
					UAVs.push_back(desc);
					break;
				}
				default:
				{
					GLEAM_ASSERT(false, "DirectX: HLSL resource type is not supported!");
					break;
				}
			}
		}

		auto comparator = [](const D3D12_SHADER_INPUT_BIND_DESC& left, const D3D12_SHADER_INPUT_BIND_DESC& right)
		{
			return left.BindPoint < right.BindPoint;
		};

		std::sort(SRVs.begin(), SRVs.end(), comparator);
		std::sort(CBVs.begin(), CBVs.end(), comparator);
		std::sort(UAVs.begin(), UAVs.end(), comparator);
	}

	~Reflection()
	{
		reflection->Release();
	}

	static const D3D12_SHADER_INPUT_BIND_DESC& GetResourceFromTypeArray(const TArray<D3D12_SHADER_INPUT_BIND_DESC>& resources, uint32_t slot)
	{
		if (resources.empty())
		{
			GLEAM_ASSERT(false, "DirectX: Shader does not use any resource");
			return invalidResource;
		}

		uint32_t left = 0;
		uint32_t right = static_cast<uint32_t>(resources.size() - 1);
		while (left <= right)
		{
			uint32_t middle = (right + left) / 2;

			const auto& resource = resources[middle];
			if (resource.BindPoint == slot) { return resource; }

			if (resource.BindPoint < slot) { left = middle + 1; }
			else { right = middle - 1; }
		}

		GLEAM_ASSERT(false, "DirectX: Resource reflection not found in slot");
		return invalidResource;
	}
};

} // namespace Gleam
