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

struct VulkanRenderPass
{
	VkRenderPass handle{ VK_NULL_HANDLE };
	bool swapchainTarget = false;

	bool operator==(const VulkanRenderPass& other) const
	{
		return handle == other.handle;
	}
};

class VulkanPipelineStateManager
{
public:

	static const VulkanRenderPass& GetRenderPass(const RenderPassDescriptor& renderPassDesc);

	static const VulkanPipeline& GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program, const VulkanRenderPass& renderPass);

	static void Clear();

private:

	struct RenderPassCacheElement
	{
		RenderPassDescriptor renderPassDescriptor;
		VulkanRenderPass renderPass;
	};

	struct GraphicsPipelineCacheElement
	{
		PipelineStateDescriptor pipelineStateDescriptor;
		GraphicsShader program;
		VulkanPipeline pipeline;
		VulkanRenderPass renderPass;
	};

	static VkRenderPass CreateRenderPass(const RenderPassDescriptor& renderPassDesc);

	static VulkanPipeline CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program, VkRenderPass renderPass);

	static inline TArray<RenderPassCacheElement> mRenderPassCache;

	static inline TArray<GraphicsPipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam