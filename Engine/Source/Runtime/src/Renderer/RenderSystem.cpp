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

void RenderSystem::Render(const World* world)
{
#ifdef USE_METAL_RENDERER
    @autoreleasepool
#endif
    {
        RenderGraph graph(mDevice.get());
        RenderGraphBlackboard blackboard;

		SceneRenderingData sceneData;
		sceneData.backbuffer = graph.ImportBackbuffer(mRenderTarget);
		sceneData.sceneProxy = world->GetSystem<RenderSceneProxy>();
		sceneData.world = world;

		if (auto camera = sceneData.sceneProxy->GetActiveCamera(); camera)
		{
			const auto& cameraComponent = camera->GetComponent<Camera>();
			sceneData.camera.viewMatrix = Float4x4::LookTo(camera->GetWorldPosition(), camera->ForwardVector(), camera->UpVector());

			if (cameraComponent.projectionType == ProjectionType::Perspective)
			{
				sceneData.camera.projectionMatrix = Float4x4::Perspective(cameraComponent.fov, cameraComponent.aspectRatio, cameraComponent.nearPlane, cameraComponent.farPlane);
			}
			else
			{
				float width = cameraComponent.orthographicSize * cameraComponent.aspectRatio;
				float height = cameraComponent.orthographicSize;
				sceneData.camera.projectionMatrix = Float4x4::Ortho(width, height, cameraComponent.nearPlane, cameraComponent.farPlane);
			}

			sceneData.camera.viewProjectionMatrix = sceneData.camera.projectionMatrix * sceneData.camera.viewMatrix;
			sceneData.camera.invViewMatrix = Math::Inverse(sceneData.camera.viewMatrix);
			sceneData.camera.invProjectionMatrix = Math::Inverse(sceneData.camera.projectionMatrix);
			sceneData.camera.invViewProjectionMatrix = Math::Inverse(sceneData.camera.viewProjectionMatrix);
			sceneData.camera.worldPosition = camera->GetWorldPosition();
		}

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
		mDevice->GetUploadManager()->Reset(frameIdx);

		cmd->Begin();

        graph.Execute(cmd);
		mDevice->GetUploadManager()->Commit();

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
