#include "gpch.h"
#include "GraphicsDevice.h"
#include "Swapchain.h"

using namespace Gleam;

GraphicsDevice::GraphicsDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue)
	: mSurface(surface)
	, mReleaseQueue(releaseQueue)
{

}

Shader GraphicsDevice::CreateShader(const TString& entryPoint, ShaderStage stage)
{
    for (const auto& shader : mShaderCache)
    {
        if (shader.GetEntryPoint() == entryPoint)
        {
            return shader;
        }
    }
    return mShaderCache.emplace_back(CompileShader(entryPoint, stage));
}

ComputePipelineHandle GraphicsDevice::CreateComputePipeline(const ComputePipelineStateDescriptor& pipelineDesc)
{
	ComputePipelineHandle handle{ eastl::hash<ComputePipelineStateDescriptor>()(pipelineDesc) };
	auto it = mComputePipelineCache.find(handle);
	if (it != mComputePipelineCache.end())
	{
		return handle;
	}

	auto pipeline = CompileComputePipeline(pipelineDesc);
	mShaderPipelineReferences[pipelineDesc.entryPoint].insert(handle);
	mComputePipelineCache.emplace_hint(mComputePipelineCache.end(), handle, pipeline);
	return handle;
}

GraphicsPipelineHandle GraphicsDevice::CreateGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc)
{
	GraphicsPipelineHandle handle{ eastl::hash<GraphicsPipelineStateDescriptor>()(pipelineDesc) };
	auto it = mGraphicsPipelineCache.find(handle);
	if (it != mGraphicsPipelineCache.end())
	{
		return handle;
	}

	auto pipeline = CompileGraphicsPipeline(pipelineDesc);
	mShaderPipelineReferences[pipelineDesc.vertexEntry].insert(handle);
	mShaderPipelineReferences[pipelineDesc.fragmentEntry].insert(handle);
	mGraphicsPipelineCache.emplace_hint(mGraphicsPipelineCache.end(), handle, pipeline);
	return handle;
}

RayTracingPipelineHandle GraphicsDevice::CreateRayTracingPipeline(const RayTracingPipelineStateDescriptor& pipelineDesc)
{
	RayTracingPipelineHandle handle{ eastl::hash<RayTracingPipelineStateDescriptor>()(pipelineDesc) };
	auto it = mRayTracingPipelineCache.find(handle);
	if (it != mRayTracingPipelineCache.end())
	{
		return handle;
	}

	auto pipeline = CompileRayTracingPipeline(pipelineDesc);
	mShaderPipelineReferences[pipelineDesc.rayGenerationEntry].insert(handle);

	for (const auto& missEntry : pipelineDesc.missEntries)
	{
		if (not missEntry.empty())
		{
			mShaderPipelineReferences[missEntry].insert(handle);
		}
	}

	for (const auto& hitGroup : pipelineDesc.hitGroups)
	{
		if (not hitGroup.closestHitEntry.empty())
		{
			mShaderPipelineReferences[hitGroup.closestHitEntry].insert(handle);
		}

		if (not hitGroup.anyHitEntry.empty())
		{
			mShaderPipelineReferences[hitGroup.anyHitEntry].insert(handle);
		}

		if (not hitGroup.intersectionEntry.empty())
		{
			mShaderPipelineReferences[hitGroup.intersectionEntry].insert(handle);
		}
	}
	mRayTracingPipelineCache.emplace_hint(mRayTracingPipelineCache.end(), handle, pipeline);
	return handle;
}

const ComputePipeline& GraphicsDevice::GetComputePipeline(ComputePipelineHandle handle) const
{
	auto it = mComputePipelineCache.find(handle);
	if (it != mComputePipelineCache.end())
	{
		return it->second;
	}

	GLEAM_ASSERT(false);
	static ComputePipeline invalid;
	return invalid;
}

const GraphicsPipeline& GraphicsDevice::GetGraphicsPipeline(GraphicsPipelineHandle handle) const
{
	auto it = mGraphicsPipelineCache.find(handle);
	if (it != mGraphicsPipelineCache.end())
	{
		return it->second;
	}

	GLEAM_ASSERT(false);
	static GraphicsPipeline invalid;
	return invalid;
}

const RayTracingPipeline& GraphicsDevice::GetRayTracingPipeline(RayTracingPipelineHandle handle) const
{
	auto it = mRayTracingPipelineCache.find(handle);
	if (it != mRayTracingPipelineCache.end())
	{
		return it->second;
	}

	GLEAM_ASSERT(false);
	static RayTracingPipeline invalid;
	return invalid;
}

void GraphicsDevice::Dispose(ResourceReleaseQueue::ObjectDeallocator&& deallocator)
{
	mReleaseQueue->AddResource(eastl::move(deallocator), static_cast<Swapchain*>(mSurface)->GetFrameIndex());
}