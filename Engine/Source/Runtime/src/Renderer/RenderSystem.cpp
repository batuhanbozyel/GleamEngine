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

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Events/RendererEvent.h"

#include "World/World.h"
#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void RenderSystem::Initialize(Engine* engine)
{
	mEngine = engine;
    mDevice = GraphicsDevice::Create();
	mUploadManager = CreateScope<UploadManager>(mDevice.get());
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
	mUploadManager.reset();
    
    for (auto renderer : mRenderers)
    {
        renderer->OnDestroy(mDevice.get());
        delete renderer;
    }

    mDevice->DestroyResources();
    mDevice.reset();
}

void RenderSystem::Render(const World* world)
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
            passData.sceneProxy = world->GetSystem<RenderSceneProxy>();
            passData.world = world;
        },
        [this](const CommandBuffer* cmd, const SceneRenderingData& passData)
        {
            CameraUniforms cameraData;
            if (auto camera = passData.sceneProxy->GetActiveCamera(); camera)
            {
				const auto& cameraComponent = camera->GetComponent<Camera>();
                cameraData.viewMatrix = Float4x4::LookTo(camera->GetWorldPosition(), camera->ForwardVector(), camera->UpVector());

				if (cameraComponent.projectionType == ProjectionType::Perspective)
				{
					cameraData.projectionMatrix = Float4x4::Perspective(cameraComponent.fov, cameraComponent.aspectRatio, cameraComponent.nearPlane, cameraComponent.farPlane);
				}
				else
				{
					float width = cameraComponent.orthographicSize * cameraComponent.aspectRatio;
					float height = cameraComponent.orthographicSize;
					cameraData.projectionMatrix = Float4x4::Ortho(width, height, cameraComponent.nearPlane, cameraComponent.farPlane);
				}

                cameraData.viewProjectionMatrix = cameraData.projectionMatrix * cameraData.viewMatrix;
                cameraData.invViewMatrix = Math::Inverse(cameraData.viewMatrix);
                cameraData.invProjectionMatrix = Math::Inverse(cameraData.projectionMatrix);
                cameraData.invViewProjectionMatrix = Math::Inverse(cameraData.viewProjectionMatrix);
                cameraData.worldPosition = camera->GetWorldPosition();
            }
            cmd->SetBufferData(passData.cameraBuffer, cameraData);
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

		mUploadManager->Commit();
        mDevice->Present(cmd);
    }
}

void RenderSystem::Configure(const RendererConfig& config)
{
	mEngine->UpdateConfig(config);
    mDevice->Configure(config);
    mCommandBuffers.resize(mDevice->GetFramesInFlight());
	for (auto& cmd : mCommandBuffers)
	{
		cmd = CreateScope<CommandBuffer>(mDevice.get());
	}
}

GraphicsDevice* RenderSystem::GetDevice()
{
    return mDevice.get();
}

const GraphicsDevice* RenderSystem::GetDevice() const
{
    return mDevice.get();
}

UploadManager* RenderSystem::GetUploadManager()
{
	return mUploadManager.get();
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
