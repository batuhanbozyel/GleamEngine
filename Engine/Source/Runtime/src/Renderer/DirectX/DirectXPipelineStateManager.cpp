#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "DirectXPipelineStateManager.h"
#include "DirectXShaderReflect.h"
#include "DirectXUtils.h"

using namespace Gleam;

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

void DirectXPipelineStateManager::Init(DirectXDevice* device)
{
	mDevice = device;
    auto samplerSates = SamplerState::GetAllVariations();
	// TODO: Create samplers (or static samplers?)
}

void DirectXPipelineStateManager::Destroy()
{
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

	// Shader stages
	psoDesc.VS; // TODO:
	psoDesc.PS; // TODO:

	// Input assembly state
	psoDesc.PrimitiveTopologyType = PrimitiveToplogyToD3D12_PRIMITIVE_TOPOLOGY_TYPE(pipelineDesc.topology);

	// Rasterizer state
	auto& rasterizerState = psoDesc.RasterizerState;
	// TODO: 

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
	psoDesc.NumRenderTargets = colorAttachments.size();
	for (uint32_t i = 0; i < colorAttachments.size(); i++)
	{
		const auto& attachment = colorAttachments[i];
		psoDesc.RTVFormats[i] = TextureFormatToDXGI_FORMAT(attachment.format);

		auto& blendState = psoDesc.BlendState;
		blendState.RenderTarget[i].BlendEnable = pipelineDesc.blendState.enabled;
		blendState.RenderTarget[i].srcColorBlendFactor = BlendModeToVkBlendFactor(pipelineDesc.blendState.sourceColorBlendMode);
		blendState.RenderTarget[i].dstColorBlendFactor = BlendModeToVkBlendFactor(pipelineDesc.blendState.destinationColorBlendMode);
		blendState.RenderTarget[i].srcAlphaBlendFactor = BlendModeToVkBlendFactor(pipelineDesc.blendState.sourceAlphaBlendMode);
		blendState.RenderTarget[i].dstAlphaBlendFactor = BlendModeToVkBlendFactor(pipelineDesc.blendState.destinationAlphaBlendMode);
		blendState.RenderTarget[i].colorBlendOp = BlendOpToVkBlendOp(pipelineDesc.blendState.colorBlendOperation);
		blendState.RenderTarget[i].alphaBlendOp = BlendOpToVkBlendOp(pipelineDesc.blendState.alphaBlendOperation);
		blendState.RenderTarget[i].colorWriteMask = ColorWriteMaskToVkColorComponentFlags(pipelineDesc.blendState.writeMask);
	}

	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline)));
    return pipeline;
}
#endif
