#include "gpch.h"
#include "RendererContext.h"
#include "RenderPipeline.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBlackboard.h"

using namespace Gleam;

void RendererContext::Execute(RenderPipeline& pipeline) const
{
#ifdef USE_METAL_RENDERER
    @autoreleasepool
#endif
    {
        TextureDescriptor descriptor;
        descriptor.size = GetDrawableSize();
        descriptor.sampleCount = mConfiguration.sampleCount;
        
        descriptor.format = TextureFormat::R32G32B32A32_SFloat;
        auto colorTarget = CreateRef<RenderTexture>(descriptor);
        
        descriptor.format = TextureFormat::D32_SFloat;
        auto depthTarget = CreateRef<RenderTexture>(descriptor);
        
        RenderGraph graph;
        RenderGraphBlackboard blackboard;
        
        RenderingData renderingData;
        renderingData.colorTarget = graph.ImportBackbuffer(colorTarget);
        renderingData.depthTarget = graph.ImportBackbuffer(depthTarget);
        blackboard.Add(renderingData);
        
        for (auto renderer : pipeline)
        {
            renderer->AddRenderPasses(graph, blackboard);
        }
        
        graph.Compile();
        graph.Execute(mCommandBuffer.get());
        mCommandBuffer->Present();
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
