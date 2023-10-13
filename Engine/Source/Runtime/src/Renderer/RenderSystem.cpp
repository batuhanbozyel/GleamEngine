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

#include "Core/Events/RendererEvent.h"

using namespace Gleam;

void RenderSystem::Initialize()
{
    mRendererContext.ConfigureBackend();
    AddRenderer<WorldRenderer>();
    AddRenderer<PostProcessStack>();
    
    EventDispatcher<RendererResizeEvent>::Subscribe([this](RendererResizeEvent e)
    {
        mRendererContext.Clear();
    });
}

void RenderSystem::Shutdown()
{
	mRendererContext.WaitDeviceIdle();
    for (auto renderer : mRenderers)
    {
        renderer->OnDestroy();
        delete renderer;
    }

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
        
        // reset rt to swapchain
        if (mRenderTarget.IsValid())
        {
            mRendererContext.ReleaseTexture(mRenderTarget);
        }
        mRenderTarget = Texture();
        
		cmd.Present();
    }
}

void RenderSystem::Configure(const RendererConfig& config)
{
    mConfiguration = config;
    mRendererContext.WaitDeviceIdle();
    mRendererContext.Clear();
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
    GLEAM_ASSERT(mRenderTarget.IsValid());
}

void RenderSystem::SetRenderTarget(const Texture& texture)
{
    mRenderTarget = texture;
}
