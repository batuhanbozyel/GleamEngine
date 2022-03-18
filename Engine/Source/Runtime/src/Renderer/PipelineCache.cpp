#include "gpch.h"
#include "PipelineCache.h"

using namespace Gleam;

/************************************************************************/
/*    Init                                                              */
/************************************************************************/
void PipelineCache::Init()
{

}
/************************************************************************/
/*    Destroy                                                           */
/************************************************************************/
void PipelineCache::Destroy()
{
	Flush();
}
/************************************************************************/
/*    CreateGraphicsPipeline                                            */
/************************************************************************/
Ref<GraphicsPipeline> PipelineCache::CreateGraphicsPipeline(const RenderPass& renderPass, const PipelineStateDescriptor& pipelineStateDescriptor, const GraphicsShader& shader)
{
	// 	auto cacheIt = mGraphicsPipelineCache.find(pipelineStateDescriptor);
	// 	if (cacheIt != mGraphicsPipelineCache.end())
	// 	{
	// 		return cacheIt->second;
	// 	}

	return CreateRef<GraphicsPipeline>(renderPass, pipelineStateDescriptor, shader);
}
/************************************************************************/
/*    Flush                                                             */
/************************************************************************/
void PipelineCache::Flush()
{
	//mGraphicsPipelineCache.clear();
}