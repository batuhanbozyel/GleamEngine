//
//  RenderSystem.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 24.06.2023.
//

#include "gpch.h"
#include "RenderSystem.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBlackboard.h"

#include "Core/Events/RendererEvent.h"

using namespace Gleam;

void RenderSystem::Initialize()
{
    mDevice = GraphicsDevice::Create();
	EventDispatcher<RendererResizeEvent>::Subscribe([this](RendererResizeEvent e)
	{
        const auto& cmd = mCommandBuffers[mDevice->GetLastFrameIndex()];
        if (cmd)
        {
            cmd->WaitUntilCompleted();
        }
		mDevice->DestroyPooledObjects();
        mDevice->DestroySizeDependentResources();
	});
}

void RenderSystem::Shutdown()
{
    mCommandBuffers[mDevice->GetLastFrameIndex()]->WaitUntilCompleted();
    mCommandBuffers.clear();
    
    for (auto renderer : mRenderers)
    {
        renderer->OnDestroy(mDevice.get());
        delete renderer;
    }

    mDevice->DestroyResources();
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
        
        const auto& sceneData = graph.AddRenderPass<SceneRenderingData>("SceneRenderingData", [&](RenderGraphBuilder& builder, SceneRenderingData& passData)
        {
            BufferDescriptor bufferDesc;
            bufferDesc.name = "CameraBuffer";
            bufferDesc.size = sizeof(CameraUniforms);
            
            passData.cameraBuffer = builder.CreateBuffer(bufferDesc);
            passData.cameraBuffer = builder.WriteBuffer(passData.cameraBuffer);
            
            passData.backbuffer = graph.ImportBackbuffer(mRenderTarget);
        },
        [this](const CommandBuffer* cmd, const SceneRenderingData& passData)
        {
            cmd->SetBufferData(passData.cameraBuffer, mCameraData);
        });
        blackboard.Add(sceneData);

        for (auto renderer : mRenderers)
        {
            renderer->AddRenderPasses(graph, blackboard);
        }
        graph.Compile();

		auto frameIdx = mDevice->GetFrameIndex();
        const auto cmd = mCommandBuffers[frameIdx].get();

		cmd->WaitUntilCompleted();
		mDevice->DestroyPooledObjects(frameIdx);

		cmd->Begin();
        graph.Execute(cmd);

        // reset rt to swapchain
        if (mRenderTarget.IsValid())
        {
            mDevice->ReleaseTexture(mRenderTarget);
        }
        ResetRenderTarget();

        mDevice->Present(cmd);
    }
}

void RenderSystem::Configure(const RendererConfig& config)
{
    Globals::Engine->UpdateConfig(config);
    
    mDevice->Configure(config);
    mCommandBuffers.resize(mDevice->GetFramesInFlight());
	for (auto& cmd : mCommandBuffers)
	{
		cmd = CreateScope<CommandBuffer>(mDevice.get());
	}
}

void RenderSystem::UpdateCamera(const Camera& camera)
{
    mCameraData.viewMatrix = camera.GetViewMatrix();
    mCameraData.projectionMatrix = camera.GetProjectionMatrix();
    mCameraData.viewProjectionMatrix = mCameraData.projectionMatrix * mCameraData.viewMatrix;
    mCameraData.invViewMatrix = Math::Inverse(mCameraData.viewMatrix);
    mCameraData.invProjectionMatrix = Math::Inverse(mCameraData.projectionMatrix);
    mCameraData.invViewProjectionMatrix = Math::Inverse(mCameraData.viewProjectionMatrix);
    mCameraData.worldPosition = camera.GetWorldPosition();
}

GraphicsDevice* RenderSystem::GetDevice()
{
    return mDevice.get();
}

const GraphicsDevice* RenderSystem::GetDevice() const
{
    return mDevice.get();
}

const Texture& RenderSystem::GetRenderTarget() const
{
    return mRenderTarget;
}

void RenderSystem::SetBackbuffer(const TextureDescriptor& descriptor)
{
    mRenderTarget = mDevice->CreateTexture(descriptor);
    GLEAM_ASSERT(mRenderTarget.IsValid());
}

void RenderSystem::SetBackbuffer(const Texture& texture)
{
    mRenderTarget = texture;
}

void RenderSystem::ResetRenderTarget()
{
    SetBackbuffer(mDevice->GetRenderSurface());
}
