//
//  RenderSystem.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 24.06.2023.
//

#include "gpch.h"
#include "RenderSystem.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBlackboard.h"

#include "Renderers/WorldRenderer.h"
#include "Renderers/PostProcessStack.h"

using namespace Gleam;

void RenderSystem::Initialize()
{
    mRendererContext.ConfigureBackend();
    mRenderTarget = CreateRef<RenderTexture>();
    mCommandBuffer = CreateScope<CommandBuffer>();
    
    AddRenderer<WorldRenderer>();
    AddRenderer<PostProcessStack>();
}

void RenderSystem::Shutdown()
{
    for (auto renderer : mRenderers)
    {
        renderer->OnDestroy();
        delete renderer;
    }
    
    mCommandBuffer.reset();
    mRenderTarget.reset();
    
    mRendererContext.DestroyBackend();
}

void RenderSystem::Render()
{
#ifdef USE_METAL_RENDERER
    @autoreleasepool
#endif
    {
        RenderGraph graph;
        RenderGraphBlackboard blackboard;
        
        RenderingData renderingData;
        renderingData.backbuffer = graph.ImportBackbuffer(mRenderTarget);
        renderingData.config = mConfiguration;
        blackboard.Add(renderingData);
        
        for (auto renderer : mRenderers)
        {
            renderer->AddRenderPasses(graph, blackboard);
        }
        
        graph.Compile();
        graph.Execute(mCommandBuffer.get());
        mCommandBuffer->Present();
        
        // reset rt to swapchain
        mRenderTarget = CreateRef<RenderTexture>();
    }
}

void RenderSystem::Configure(const RendererConfig& config)
{
    mConfiguration = config;
    mRendererContext.Configure(config);
}

const RendererConfig& RenderSystem::GetConfiguration() const
{
    return mConfiguration;
}
