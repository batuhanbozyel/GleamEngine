#include "gpch.h"
#include "RendererContext.h"
#include "Renderer.h"
#include "RenderGraph/RenderGraph.h"

using namespace Gleam;

void RendererContext::Exectute(PolyArray<IRenderer>& renderPipeline) const
{
    RenderGraph graph;
    
    RenderingData renderingData;
    renderingData.colorTarget = graph.ImportBackbuffer(GetSwapchainTarget());
    
    for (auto renderer : renderPipeline)
        renderer->AddRenderPasses(graph, renderingData);
    
    graph.Compile();
    graph.Execute(mCommandBuffer);
    mCommandBuffer.Present();
}

const RefCounted<Shader>& RendererContext::CreateShader(const TString& entryPoint, ShaderStage stage)
{
    for (const auto& shader : mShaderCache)
    {
        if (shader->GetEntryPoint() == entryPoint)
            return shader;
    }
    
    return mShaderCache.emplace_back(CreateRef<Shader>(entryPoint, stage));
}

const RendererConfig& RendererContext::GetConfiguration() const
{
	return mConfiguration;
}
