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
    mDevice = GraphicsDevice::Create();
    AddRenderer<WorldRenderer>();
    AddRenderer<PostProcessStack>();

    EventDispatcher<RendererResizeEvent>::Subscribe([this](RendererResizeEvent e)
    {
        mDevice->Clear();
    });
}

void RenderSystem::Shutdown()
{
    mDevice->WaitDeviceIdle();
    for (auto renderer : mRenderers)
    {
        renderer->OnDestroy(mDevice.get());
        delete renderer;
    }

    mDevice->Clear();
    mDevice.reset();
}

void RenderSystem::Render()
{
#ifdef USE_METAL_RENDERER
    @autoreleasepool
#endif
    {
        RenderGraph graph(mDevice.get());
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

        CommandBuffer cmd(mDevice.get());
        graph.Execute(&cmd);

        // reset rt to swapchain
        if (mRenderTarget.IsValid())
        {
            mDevice->ReleaseTexture(mRenderTarget);
        }
        ResetRenderTarget();

        cmd.Present();
    }
}

void RenderSystem::Configure(const RendererConfig& config)
{
    mConfiguration = config;
    mDevice->WaitDeviceIdle();
    mDevice->Clear();
    mDevice->Configure(config);
}

GraphicsDevice* RenderSystem::GetDevice()
{
    return mDevice.get();
}

const GraphicsDevice* RenderSystem::GetDevice() const
{
    return mDevice.get();
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
    mRenderTarget = mDevice->CreateTexture(descriptor);
    GLEAM_ASSERT(mRenderTarget.IsValid());
}

void RenderSystem::SetRenderTarget(const Texture& texture)
{
    mRenderTarget = texture;
}

void RenderSystem::ResetRenderTarget()
{
    SetRenderTarget(mDevice->GetSwapchain()->GetTexture());
}
