#pragma once
#include "Renderer/Shader.h"
#include "DirectXDevice.h"

#include <d3d12shader.h>
#include <dxcapi.h>

namespace Gleam {

struct Shader::Reflection
{
	const DirectXDevice* device;
	ID3D12ShaderReflection* reflection;

	Reflection(const DirectXDevice* device, const D3D12_SHADER_BYTECODE* shaderBytecode)
		: device(device)
	{
		DxcBuffer reflectionBuffer{};
		reflectionBuffer.Ptr = shaderBytecode->pShaderBytecode;
		reflectionBuffer.Size = shaderBytecode->BytecodeLength;
		reflectionBuffer.Encoding = DXC_CP_ACP;
		device->GetDxcUtils()->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection));
	}

	~Reflection()
	{
		reflection->Release();
	}
};

} // namespace Gleam
