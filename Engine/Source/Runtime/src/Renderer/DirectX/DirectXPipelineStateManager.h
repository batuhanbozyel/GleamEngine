#pragma once
#ifdef USE_DIRECTX_RENDERER
#include "DirectXDevice.h"
#include "Renderer/Shader.h"
#include "Renderer/SamplerState.h"

namespace Gleam {

struct DirectXPipeline
{
	ID3D12PipelineState* handle = nullptr;
};

struct DirectXGraphicsPipeline : public DirectXPipeline
{
	Shader vertexShader;
	Shader fragmentShader;
};

class DirectXPipelineStateManager
{
public:

	static void Init(DirectXDevice* device);

	static void Destroy();

	static const DirectXGraphicsPipeline* GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount);

	static const DirectXGraphicsPipeline* GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount);

	static ID3D12RootSignature* GetGlobalRootSignature();

	static void Clear();

private:

	static ID3D12PipelineState* CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount);
    
    static inline HashMap<size_t, Scope<DirectXGraphicsPipeline>> mGraphicsPipelineCache;

	static inline ID3D12RootSignature* mRootSignature = nullptr;

	static inline DirectXDevice* mDevice = nullptr;

};

} // namespace Gleam
#endif
