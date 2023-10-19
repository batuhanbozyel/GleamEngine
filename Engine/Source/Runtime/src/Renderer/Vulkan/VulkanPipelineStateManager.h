#pragma once
#ifdef USE_VULKAN_RENDERER
#include "VulkanUtils.h"
#include "VulkanDevice.h"
#include "Renderer/Shader.h"
#include "Renderer/SamplerState.h"

namespace Gleam {

struct VulkanPipeline
{
	VkPipeline handle{ VK_NULL_HANDLE };
	VkPipelineLayout layout{ VK_NULL_HANDLE };
	VkPipelineBindPoint bindPoint{ VK_PIPELINE_BIND_POINT_GRAPHICS };
};

struct VulkanGraphicsPipeline : public VulkanPipeline
{
	Shader vertexShader;
	Shader fragmentShader;
};

class VulkanPipelineStateManager
{
public:

	static void Init(VulkanDevice* device);

	static void Destroy();

	static const VulkanGraphicsPipeline* GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const Shader& vertexShader, const Shader& fragmentShader, VkRenderPass renderPass, uint32_t sampleCount);

	static const VulkanGraphicsPipeline* GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, VkRenderPass renderPass, uint32_t sampleCount);

	static VkSampler GetSampler(uint32_t index);

	static VkSampler GetSampler(const SamplerState& samplerState);

	static void Clear();

private:
    
    static VkPipelineLayout CreatePipelineLayout(const Shader& vertexShader, const Shader& fragmentShader);

	static VkPipeline CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, VkRenderPass renderPass, VkPipelineLayout layout, uint32_t sampleCount);

	static inline TArray<VkSampler, 12> mSamplerStates;
    
    static inline HashMap<size_t, Scope<VulkanGraphicsPipeline>> mGraphicsPipelineCache;

	static inline VulkanDevice* mDevice = nullptr;

};

} // namespace Gleam
#endif
