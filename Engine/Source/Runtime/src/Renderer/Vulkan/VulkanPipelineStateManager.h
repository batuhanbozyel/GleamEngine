#pragma once
#ifdef USE_VULKAN_RENDERER
#include "VulkanUtils.h"
#include "Renderer/Shader.h"

namespace Gleam {

struct VulkanPipeline
{
	VkPipeline handle{ VK_NULL_HANDLE };
	VkPipelineLayout layout{ VK_NULL_HANDLE };
	VkPipelineBindPoint bindPoint{ VK_PIPELINE_BIND_POINT_GRAPHICS };
	PipelineStateDescriptor descriptor;
};

struct VulkanGraphicsPipeline : public VulkanPipeline
{
	RefCounted<Shader> vertexShader;
	RefCounted<Shader> fragmentShader;
};

class VulkanPipelineStateManager
{
public:

	static void Init();

	static void Destroy();

	static const VulkanGraphicsPipeline* GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc,
													 const TArray<TextureDescriptor>& colorAttachments,
													 const RefCounted<Shader>& vertexShader,
													 const RefCounted<Shader>& fragmentShader,
													 VkRenderPass renderPass,
													 uint32_t sampleCount);

	static const VulkanGraphicsPipeline* GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc,
													 const TArray<TextureDescriptor>& colorAttachments,
													 const TextureDescriptor& depthAttachment,
													 const RefCounted<Shader>& vertexShader,
													 const RefCounted<Shader>& fragmentShader,
													 VkRenderPass renderPass,
													 uint32_t sampleCount);

	static VkSampler GetSampler(uint32_t index);

	static VkSampler GetSampler(const SamplerState& samplerState);

	static void Clear();

private:

	struct GraphicsPipelineCacheElement
	{
		VulkanGraphicsPipeline pipeline;
		TArray<TextureDescriptor> colorAttachments;
		TextureDescriptor depthAttachment;
		bool hasDepthAttachment = false;
		uint32_t sampleCount = 1;
	};

	static void CreateGraphicsPipeline(GraphicsPipelineCacheElement& element, VkRenderPass renderPass);

	static inline TArray<VkSampler, 12> mSamplerStates;
	static inline TArray<GraphicsPipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam
#endif
