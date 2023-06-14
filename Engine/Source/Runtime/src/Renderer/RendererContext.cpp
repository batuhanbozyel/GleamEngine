#include "gpch.h"
#include "RendererContext.h"
#include "RenderPipeline.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBlackboard.h"

using namespace Gleam;

void RendererContext::Execute(RenderPipeline* pipeline) const
{
#ifdef USE_METAL_RENDERER
    @autoreleasepool
#endif
    {
        RenderGraph graph;
        RenderGraphBlackboard blackboard;
        
        RenderingData renderingData;
        renderingData.backbuffer = graph.ImportBackbuffer(pipeline->mRenderTarget);
        blackboard.Add(renderingData);
        
        for (auto renderer : *pipeline)
        {
            renderer->AddRenderPasses(graph, blackboard);
        }
        
        graph.Compile();
        graph.Execute(mCommandBuffer.get());
        mCommandBuffer->Present();
        
        pipeline->SetRenderTarget(CreateRef<RenderTexture>());
    }
}

const RefCounted<Shader>& RendererContext::CreateShader(const TString& entryPoint, ShaderStage stage)
{
    for (const auto& shader : mShaderCache)
    {
        if (shader->GetEntryPoint() == entryPoint)
        {
            return shader;
        }
    }
    
    return mShaderCache.emplace_back(CreateRef<Shader>(entryPoint, stage));
}

const RendererConfig& RendererContext::GetConfiguration() const
{
	return mConfiguration;
}
