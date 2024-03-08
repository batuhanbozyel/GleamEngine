#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "DirectXPipelineStateManager.h"
#include "DirectXShaderReflect.h"
#include "DirectXUtils.h"

using namespace Gleam;

uint32_t PushConstantRootParameterIndex = 0;

static size_t PipelineHasher(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount)
{
    size_t hash = 0;
    hash_combine(hash, pipelineDesc);
    hash_combine(hash, vertexShader);
    hash_combine(hash, fragmentShader);
    for (const auto& colorAttachment : colorAttachments)
    {
        hash_combine(hash, colorAttachment.format);
    }
    hash_combine(hash, depthAttachment.format);
    hash_combine(hash, sampleCount);
    return hash;
}

static D3D12_STATIC_SAMPLER_DESC CreateStaticSampler(const SamplerState& samplerState)
{
	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 1;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = 16.0f;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	switch (samplerState.filterMode)
	{
		case FilterMode::Point:
		{
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			break;
		}
		case FilterMode::Bilinear:
		{
			sampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		}
		case FilterMode::Trilinear:
		{
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		}
		default: GLEAM_ASSERT(false, "DirectX: Filter mode is not supported!") break;
	}

	switch (samplerState.wrapMode)
	{
		case WrapMode::Repeat:
		{
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			break;
		}
		case WrapMode::Clamp:
		{
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			break;
		}
		case WrapMode::Mirror:
		{
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			break;
		}
		case WrapMode::MirrorOnce:
		{
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			break;
		}
		default: GLEAM_ASSERT(false, "DirectX: Wrap mode is not supported!") break;
	}

	return sampler;
}

void DirectXPipelineStateManager::Init(DirectXDevice* device)
{
	mDevice = device;

	// Static samplers
	std::hash<SamplerState> hasher;
    auto samplerSates = SamplerState::GetAllVariations();
	for (uint32_t i = 0; i < samplerSates.size(); i++)
	{
		const auto& sampler = samplerSates[i];
		mStaticSamplerDescs[i] = CreateStaticSampler(sampler);
		mStaticSamplerDescs[i].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		mStaticSamplerDescs[i].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		mStaticSamplerDescs[i].ShaderRegister = (UINT)hasher(sampler);
	}
    
    // Root signature
    constexpr uint32_t NumRootParams = PUSH_CONSTANT_SLOT + 1;
    D3D12_ROOT_PARAMETER1 rootSigParams[NumRootParams];
    for (uint32_t i = 0; i < PUSH_CONSTANT_SLOT; i++)
    {
        rootSigParams[i] = {
          .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
          .Descriptor = {
              .ShaderRegister = i,
              .RegisterSpace = 0,
              .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE
          },
          .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        };
    }
    // Push constant
    rootSigParams[PUSH_CONSTANT_SLOT] = {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
      .Constants = {
          .ShaderRegister = PUSH_CONSTANT_REGISTER,
          .RegisterSpace = 0,
          .Num32BitValues = PUSH_CONSTANT_SIZE / sizeof(uint32_t)
      },
      .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
    };
    
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignature = {};
	rootSignature.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSignature.Desc_1_1.NumParameters = NumRootParams;
	rootSignature.Desc_1_1.pParameters = rootSigParams;
	rootSignature.Desc_1_1.NumStaticSamplers = mStaticSamplerDescs.size();
	rootSignature.Desc_1_1.pStaticSamplers = mStaticSamplerDescs.data();
	rootSignature.Desc_1_1.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

	ID3DBlob* blob = nullptr;
	ID3DBlob* error = nullptr;
	DX_CHECK(D3D12SerializeVersionedRootSignature(&rootSignature, &blob, &error));

	if (error)
	{
		char* error_msg = (char*)error->GetBufferPointer();
		GLEAM_CORE_ERROR("DirectX: Root signature error: {0}\n", error_msg);
	}

	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
	blob->Release();
}

void DirectXPipelineStateManager::Destroy()
{
	mRootSignature->Release();
	Clear();
}

const DirectXGraphicsPipeline* DirectXPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount)
{
    return GetGraphicsPipeline(pipelineDesc, colorAttachments, TextureDescriptor(), vertexShader, fragmentShader, sampleCount);
}

const DirectXGraphicsPipeline* DirectXPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount)
{
    auto key = PipelineHasher(pipelineDesc, colorAttachments, depthAttachment, vertexShader, fragmentShader, sampleCount);
    auto it = mGraphicsPipelineCache.find(key);
    if (it != mGraphicsPipelineCache.end())
    {
        return it->second.get();
    }

    auto pipeline = new DirectXGraphicsPipeline;
    pipeline->vertexShader = vertexShader;
    pipeline->fragmentShader = fragmentShader;
    pipeline->handle = CreateGraphicsPipeline(pipelineDesc, colorAttachments, depthAttachment, vertexShader, fragmentShader, sampleCount);
    mGraphicsPipelineCache.insert(mGraphicsPipelineCache.end(), {key, Scope<DirectXGraphicsPipeline>(pipeline)});
    return pipeline;
}

void DirectXPipelineStateManager::Clear()
{
	for (const auto& [_, pipeline] : mGraphicsPipelineCache)
	{
		pipeline->handle->Release();
	}
	mGraphicsPipelineCache.clear();
}

ID3D12PipelineState* DirectXPipelineStateManager::CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount)
{
	ID3D12PipelineState* pipeline = nullptr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = mRootSignature;
	psoDesc.SampleDesc.Count = sampleCount;
	psoDesc.SampleMask = UINT_MAX;

	// Shader stages
	psoDesc.VS = *static_cast<D3D12_SHADER_BYTECODE*>(vertexShader.GetHandle());
	psoDesc.PS = *static_cast<D3D12_SHADER_BYTECODE*>(fragmentShader.GetHandle());

	// Input assembly state
	psoDesc.PrimitiveTopologyType = PrimitiveToplogyToD3D12_PRIMITIVE_TOPOLOGY_TYPE(pipelineDesc.topology);
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	// Rasterizer state
	auto& rasterizerState = psoDesc.RasterizerState;
	rasterizerState.FrontCounterClockwise = FALSE;
	rasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerState.DepthClipEnable = FALSE;
	rasterizerState.MultisampleEnable = sampleCount > 1;
	rasterizerState.AntialiasedLineEnable = FALSE;
	rasterizerState.ForcedSampleCount = 0;
	rasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	rasterizerState.CullMode = CullModeToD3D12_CULL_MODE(pipelineDesc.cullingMode);

	// Depth-stencil state
	if (Utils::IsDepthFormat(depthAttachment.format))
	{
		psoDesc.DSVFormat = TextureFormatToDXGI_FORMAT(depthAttachment.format);

		auto& depthStencilState = psoDesc.DepthStencilState;
		depthStencilState.DepthEnable = pipelineDesc.depthState.compareFunction != CompareFunction::Always;
		depthStencilState.DepthFunc = CompareFunctionToD3D12_COMPARISON_FUNC(pipelineDesc.depthState.compareFunction);
		depthStencilState.DepthWriteMask = pipelineDesc.depthState.writeEnabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;

		if (Utils::IsDepthStencilFormat(depthAttachment.format))
		{
			depthStencilState.StencilEnable = pipelineDesc.stencilState.enabled;
			depthStencilState.StencilReadMask = pipelineDesc.stencilState.readMask;
			depthStencilState.StencilWriteMask = pipelineDesc.stencilState.writeMask;

			D3D12_DEPTH_STENCILOP_DESC stencilOp{};
			stencilOp.StencilFailOp = StencilOpToD3D12_STENCIL_OP(pipelineDesc.stencilState.failOperation);
			stencilOp.StencilPassOp = StencilOpToD3D12_STENCIL_OP(pipelineDesc.stencilState.passOperation);
			stencilOp.StencilDepthFailOp = StencilOpToD3D12_STENCIL_OP(pipelineDesc.stencilState.depthFailOperation);
			stencilOp.StencilFunc = CompareFunctionToD3D12_COMPARISON_FUNC(pipelineDesc.stencilState.compareFunction);

			depthStencilState.BackFace = stencilOp;
			depthStencilState.FrontFace = stencilOp;
		}
	}

	// Blend state
	auto& blendState = psoDesc.BlendState;
	blendState.AlphaToCoverageEnable = pipelineDesc.alphaToCoverage;
	blendState.IndependentBlendEnable = FALSE;

	psoDesc.NumRenderTargets = colorAttachments.size();
	for (uint32_t i = 0; i < colorAttachments.size(); i++)
	{
		const auto& attachment = colorAttachments[i];
		psoDesc.RTVFormats[i] = TextureFormatToDXGI_FORMAT(attachment.format);
		blendState.RenderTarget[i].BlendEnable = pipelineDesc.blendState.enabled;
		blendState.RenderTarget[i].SrcBlend = BlendModeToD3D12_BLEND(pipelineDesc.blendState.sourceColorBlendMode);
		blendState.RenderTarget[i].DestBlend = BlendModeToD3D12_BLEND(pipelineDesc.blendState.destinationColorBlendMode);
		blendState.RenderTarget[i].SrcBlendAlpha = BlendModeToD3D12_BLEND(pipelineDesc.blendState.sourceAlphaBlendMode);
		blendState.RenderTarget[i].DestBlendAlpha = BlendModeToD3D12_BLEND(pipelineDesc.blendState.destinationAlphaBlendMode);
		blendState.RenderTarget[i].BlendOp = BlendOpToD3D12_BLEND_OP(pipelineDesc.blendState.colorBlendOperation);
		blendState.RenderTarget[i].BlendOpAlpha = BlendOpToD3D12_BLEND_OP(pipelineDesc.blendState.alphaBlendOperation);
		blendState.RenderTarget[i].RenderTargetWriteMask = ColorWriteMaskToD3D12_COLOR_WRITE_ENABLE(pipelineDesc.blendState.writeMask);
	}

	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline)));
    return pipeline;
}

ID3D12RootSignature* DirectXPipelineStateManager::GetGlobalRootSignature()
{
	return mRootSignature;
}
#endif
