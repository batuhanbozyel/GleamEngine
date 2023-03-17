#pragma once
#ifdef USE_VULKAN_RENDERER
#include "VulkanUtils.h"
#include "Renderer/Shader.h"

namespace Gleam {

struct VulkanPipeline
{
	VkPipeline handle{ VK_NULL_HANDLE };
	VkPipelineLayout layout{ VK_NULL_HANDLE };
	VkPipelineBindPoint bindPoint{};
};

struct VulkanRenderPass
{
	VkRenderPass handle{ VK_NULL_HANDLE };
	uint32_t sampleCount = 1;
};

class VulkanPipelineStateManager
{
public:

	static const VulkanPipeline& GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, const VulkanRenderPass& renderPass);

	static void Clear();

private:

	struct GraphicsPipelineCacheElement
	{
		PipelineStateDescriptor pipelineStateDescriptor;
		RefCounted<Shader> vertexShader;
		RefCounted<Shader> fragmentShader;
		VulkanPipeline pipeline;
		uint32_t sampleCount = 1;
	};

	static VulkanPipeline CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, const VulkanRenderPass& renderPass);

	static inline TArray<GraphicsPipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam
#endif