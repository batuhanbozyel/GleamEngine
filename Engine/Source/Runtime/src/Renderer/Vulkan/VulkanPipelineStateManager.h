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

	static const VulkanPipeline& GetPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<RefCounted<Shader>>& program, const VulkanRenderPass& renderPass);

	static void Clear();

private:

	struct PipelineCacheElement
	{
		PipelineStateDescriptor pipelineStateDescriptor;
		TArray<RefCounted<Shader>> program;
		VulkanPipeline pipeline;
		uint32_t sampleCount = 1;
	};

	static VulkanPipeline CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, const VulkanRenderPass& renderPass);

	static inline TArray<PipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam
#endif