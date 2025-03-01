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
	InitializeBackend();
	mContext.device = mDevice.get();
	mContext.surface = mSwapchain.get();
	mContext.releaseQueue = mReleaseQueue.get();
	mContext.resourcePool = mResourcePool.get();

	EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
	{
		auto size = Size((float)e.GetWidth(), (float)e.GetHeight());
		if (mSwapchainSize != size)
		{
			mRendererResized = true;
			mSwapchainSize = size;
		}
		EventDispatcher<RendererResizeEvent>::Publish(RendererResizeEvent(size));
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

	mReleaseQueue->Clear();
	mResourcePool->Clear();

	mResourcePool.reset();
	mReleaseQueue.reset();
    mDevice.reset();
	mSwapchain.reset();
}

void RenderSystem::Render(const World* world)
{
#ifdef USE_METAL_RENDERER
	@autoreleasepool
#endif
	{
		auto sceneProxy = world->GetSystem<RenderSceneProxy>();
		auto camera = sceneProxy->GetActiveCamera();
		if (camera == nullptr)
		{
			return; // skip rendering this frame
		}
		const auto& cameraComponent = camera->GetComponent<Camera>();

		// TODO: Render scene per active camera
		// Set sceneTarget to camera target
		// Camera target default requires a special handle
		// which maps to backbuffer for runtime and a temporary texture for editor scene view
		// We may want to decide this on Editor side
		RenderTextureDescriptor sceneTargetDesc{};
		sceneTargetDesc.name = "Scene Target";
		sceneTargetDesc.size = cameraComponent.GetViewport();
		sceneTargetDesc.format = mSwapchain->GetFormat();

		auto sceneTarget = mResourcePool->Allocate(sceneTargetDesc);
		const auto& backbuffer = mSwapchain->GetCurrentDrawable();

		RenderGraph graph(mContext);
		RenderGraphBlackboard blackboard;

		auto& sceneData = blackboard.Add<SceneRenderingData>();
		sceneData.backbuffer = graph.ImportBackbuffer(backbuffer);
		sceneData.sceneTarget = graph.ImportBackbuffer(sceneTarget);
		sceneData.sceneProxy = sceneProxy;
		sceneData.world = world;
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

        for (auto renderer : mRenderers)
        {
            renderer->AddRenderPasses(graph, blackboard);
        }
        graph.Compile();

		auto frameIdx = mSwapchain->GetFrameIndex();
		const auto cmd = mCommandBuffers[frameIdx].get();

		cmd->WaitUntilCompleted();
		if (mRendererResized)
		{
			mSwapchain->Resize(mDevice.get(), mSwapchainSize);
			mResourcePool->Clear();
			mRendererResized = false;
		}

		mReleaseQueue->Flush(frameIdx);
		mDevice->ResetCommandPools(frameIdx);

		cmd->Begin();
        graph.Execute(cmd, sceneData);
		mSwapchain->Present(cmd);

		mResourcePool->Release(sceneTarget);
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
	mSwapchainSize = mSwapchain->GetCurrentDrawable().GetDescriptor().size;
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
