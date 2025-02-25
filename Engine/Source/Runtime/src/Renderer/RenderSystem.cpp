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
	InitializeBackend();

	mEngine = engine;
	mContext.device = mDevice.get();
	mContext.surface = mSwapchain.get();
	mUploadManager = CreateScope<UploadManager>(mDevice.get());

	EventDispatcher<RendererResizeEvent>::Subscribe([this](RendererResizeEvent e)
	{
        const auto& cmd = mCommandBuffers[mSwapchain->GetFrameIndex()];
        if (cmd)
        {
            cmd->WaitUntilCompleted();
        }
		mDevice->DestroyPooledObjects();
        mDevice->DestroySizeDependentResources();
		mSwapchain->Resize(mDevice.get(), e.GetSize());
	});
}

void RenderSystem::Shutdown()
{
	mUploadManager.reset();

    mCommandBuffers[mSwapchain->GetFrameIndex()]->WaitUntilCompleted();
    mCommandBuffers.clear();

    for (auto renderer : mRenderers)
    {
        renderer->OnDestroy(mContext);
        delete renderer;
    }
	mRenderers.clear();

    mDevice->DestroyResources();
    mDevice.reset();
}

void RenderSystem::Render(const World* world)
{
#ifdef USE_METAL_RENDERER
	@autoreleasepool
#endif
	{
		// TODO: Set backbuffer to swapchain image for game runtime
		RenderTextureDescriptor backbufferDesc{};
		backbufferDesc.name = "Scene Backbuffer";
		backbufferDesc.size = mSwapchain->GetSize();
		backbufferDesc.format = mSwapchain->GetFormat();

		auto backbuffer = mDevice->CreateTexture(backbufferDesc);
		auto frameIdx = mSwapchain->GetFrameIndex();
		mDevice->DestroyPooledObjects(frameIdx);

        RenderGraph graph(mDevice.get());
        RenderGraphBlackboard blackboard;

		SceneRenderingData sceneData;
		sceneData.backbuffer = graph.ImportBackbuffer(backbuffer, ImportResourceParams{ .clearColor = backbufferDesc.clearColor, .clearOnFirstUse = backbufferDesc.clearBuffer });
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
			sceneData.camera.position = camera->GetWorldPosition();
		}
		blackboard.Add(sceneData);

        for (auto renderer : mRenderers)
        {
            renderer->AddRenderPasses(graph, blackboard);
        }
        graph.Compile();

		const auto cmd = mCommandBuffers[frameIdx].get();
		cmd->Begin();
        graph.Execute(cmd);
		mSwapchain->Present(cmd);
    }
}

void RenderSystem::Configure(const RendererConfig& config)
{
	mEngine->UpdateConfig(config);
    mDevice->Configure(config);

    mCommandBuffers.resize(mSwapchain->GetFramesInFlight());
	for (auto& cmd : mCommandBuffers)
	{
		cmd = CreateScope<CommandBuffer>(mDevice.get());
	}
}

UploadManager* RenderSystem::GetUploadManager()
{
	return mUploadManager.get();
}

GraphicsDevice* RenderSystem::GetDevice()
{
    return mDevice.get();
}

const GraphicsDevice* RenderSystem::GetDevice() const
{
    return mDevice.get();
}

RenderSurface* RenderSystem::GetSurface()
{
	return mSwapchain.get();
}

const RenderSurface* RenderSystem::GetSurface() const
{
	return mSwapchain.get();
}

void RenderSystem::RecompileShader(const TString& entryPoint)
{
	mCommandBuffers[mSwapchain->GetFrameIndex()]->WaitUntilCompleted();

	for (auto& shader : mDevice->mShaderCache)
	{
		if (shader.GetEntryPoint() == entryPoint)
		{
			auto newShader = mDevice->CompileShader(shader.GetEntryPoint(), shader.GetStage());
			if (newShader.IsValid())
			{
				mDevice->Dispose(shader);
				shader = newShader;

				for (auto pipelineHash : mDevice->mShaderPipelineReferences[entryPoint])
				{
					for (auto& [handle, pipeline] : mDevice->mGraphicsPipelineCache)
					{
						if (handle == pipelineHash)
						{
							auto newPipeline = mDevice->CompileGraphicsPipeline(pipeline.GetDescriptor());
							if (newPipeline.IsValid())
							{
								mDevice->Dispose(pipeline);
								pipeline = newPipeline;
							}
							break;
						}
					}
				}
			}
			break;
		}
	}
}
