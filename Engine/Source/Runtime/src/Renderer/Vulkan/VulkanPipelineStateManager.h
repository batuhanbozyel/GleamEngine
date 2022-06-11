#pragma once
#include "VulkanUtils.h"
#include "Renderer/Shader.h"

namespace Gleam {

struct VulkanPipeline
{
	VkPipeline handle{ VK_NULL_HANDLE };
	VkPipelineLayout layout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout setLayout{ VK_NULL_HANDLE };
};

struct VulkanPipelineState
{
	VkRenderPass renderPass{ VK_NULL_HANDLE };
	VulkanPipeline pipeline;
	bool swapchainTarget = false;
};

class VulkanPipelineStateManager
{
public:

	static const VulkanPipelineState& GetGraphicsPipelineState(const RenderPassDescriptor& renderPassDesc, const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program);

	static void Clear();

private:

	struct GraphicsPipelineCacheElement
	{
		RenderPassDescriptor renderPassDescriptor;
		PipelineStateDescriptor pipelineStateDescriptor;
		GraphicsShader program;
		VulkanPipelineState pipelineState;
	};


	static VkRenderPass CreateRenderPass(const RenderPassDescriptor& renderPassDesc);

	static VulkanPipeline CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program, VkRenderPass renderPass);

	static inline TArray<GraphicsPipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam