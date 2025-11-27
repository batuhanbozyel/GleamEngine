#include "gpch.h"
#include "GraphicsDevice.h"

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