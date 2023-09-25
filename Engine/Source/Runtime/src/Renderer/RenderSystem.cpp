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
    mRendererContext.Clear();
    mRendererContext.DestroyBackend();
}

void RenderSystem::Render()
{
#ifdef USE_METAL_RENDERER
    @autoreleasepool
#endif
    {
        RenderGraph graph(mRendererContext);
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

		CommandBuffer cmd;
        graph.Execute(&cmd);
		cmd.Present();
        
        // reset rt to swapchain
        if (mRenderTarget.IsValid())
        {
            mRendererContext.ReleaseTexture(mRenderTarget);
        }
        mRenderTarget = Texture();
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

const Texture& RenderSystem::GetRenderTarget() const
{
    return mRenderTarget;
}

void RenderSystem::SetRenderTarget(const TextureDescriptor& descriptor)
{
    mRenderTarget = mRendererContext.CreateTexture(descriptor);
}

void RenderSystem::SetRenderTarget(const Texture& texture)
{
    mRenderTarget = texture;
}
