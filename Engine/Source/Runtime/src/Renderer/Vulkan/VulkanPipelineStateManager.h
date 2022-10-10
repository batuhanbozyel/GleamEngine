#pragma once
#ifdef USE_VULKAN_RENDERER
#include "VulkanUtils.h"
#include "Renderer/Shader.h"

namespace Gleam {

struct VulkanPipeline
{
	VkPipeline handle{ VK_NULL_HANDLE };
	VkPipelineLayout layout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout setLayout{ VK_NULL_HANDLE };
};

class VulkanPipelineStateManager
{
public:

	static const VulkanPipeline& GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program, VkRenderPass renderPass);

	static void Clear();

private:

	struct GraphicsPipelineCacheElement
	{
		PipelineStateDescriptor pipelineStateDescriptor;
		GraphicsShader program;
		VulkanPipeline pipeline;
	};

	static VulkanPipeline CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program, VkRenderPass renderPass);

	static inline TArray<GraphicsPipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam
#endif