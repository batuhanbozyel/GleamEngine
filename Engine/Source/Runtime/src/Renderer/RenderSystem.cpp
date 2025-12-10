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
	mContext.allocator = mPersistentAllocator.get();

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
	mCopyCommandBuffer.reset();
	mCommandBuffers.clear();

	for (auto renderer : mRenderers)
	{
		renderer->OnDestroy(mContext);
		delete renderer;
	}
	mRenderers.clear();

	mTransientAllocator.reset();
	mPersistentAllocator.reset();

	mReleaseQueue.reset();
    mDevice.reset();
	mSwapchain.reset();
}

void RenderSystem::PreRender(const World* world)
{
	auto sceneProxy = world->GetSystem<RenderSceneProxy>();
	sceneProxy->ForEach([&](const MeshBatch& batch)
	{
		mCopyCommandBuffer->Commit(batch.instanceBuffer, batch.instances.data(), sizeof(MeshInstanceData) * batch.numInstances);
	});

	// update active camera
	mActiveCamera = InvalidEntity;
	world->GetEntityManager().ForEach<Entity, Camera>([&](const Entity& entity, const Camera& component)
	{
		if (entity.IsActive())
		{
			mActiveCamera = entity;
		}
	});

	auto frameIdx = mSwapchain->GetFrameIndex();
	const auto cmd = mCommandBuffers[frameIdx].get();

	cmd->WaitUntilCompleted();
	if (mRendererResized)
	{
		mSwapchain->Resize(mDevice.get(), mSwapchainSize);
		mRendererResized = false;
	}
	mDevice->ResetCommandPools(frameIdx);
	mReleaseQueue->Flush(frameIdx);

	mTransientAllocator->CollectGarbage(mSwapchain->GetFramesInFlight() + 1);
	mPersistentAllocator->CollectGarbage(mSwapchain->GetFramesInFlight() + 1);
}

void RenderSystem::Render(const World* world)
{
#ifdef USE_METAL_RENDERER
	@autoreleasepool
#endif
	{
		if (mActiveCamera == InvalidEntity)
		{
			return; // skip rendering this frame
		}

		const auto& cameraComponent = world->GetEntityManager().GetComponent<Camera>(mActiveCamera);
		const auto& cameraEntity = world->GetEntityManager().GetComponent<Entity>(mActiveCamera);

		// TODO: Render scene per active camera
		// Set sceneTarget to camera target
		// Camera target default requires a special handle
		// which maps to backbuffer for runtime and a temporary texture for editor scene view
		// We may want to decide this on Editor side
		RenderTextureDescriptor sceneTargetDesc{};
		sceneTargetDesc.name = "Scene Target";
		sceneTargetDesc.size = cameraComponent.GetViewport();
		sceneTargetDesc.format = mSwapchain->GetFormat();

		RenderGraphContext renderGraphContext;
		renderGraphContext.device = mDevice.get();
		renderGraphContext.surface = mSwapchain.get();
		renderGraphContext.allocator = mTransientAllocator.get();
		RenderGraph graph(renderGraphContext);

		auto sceneTarget = mDevice->CreateTexture(renderGraphContext.allocator, sceneTargetDesc);
		const auto& backbuffer = mSwapchain->GetCurrentDrawable();

		RenderGraphBlackboard blackboard;
		auto& sceneData = blackboard.Add<SceneRenderingData>();
		sceneData.backbuffer = graph.ImportBackbuffer(backbuffer);
		sceneData.sceneTarget = graph.ImportBackbuffer(sceneTarget);
		sceneData.sceneProxy = world->GetSystem<RenderSceneProxy>();
		sceneData.world = world;
		sceneData.camera.resolution = Float2(cameraComponent.orthographicSize * cameraComponent.aspectRatio, cameraComponent.orthographicSize);
		sceneData.camera.viewMatrix = Float4x4::LookTo(cameraEntity.GetWorldPosition(), cameraEntity.ForwardVector(), cameraEntity.UpVector());

		if (cameraComponent.projectionType == ProjectionType::Perspective)
		{
			sceneData.camera.projectionMatrix = Float4x4::Perspective(cameraComponent.fov, cameraComponent.aspectRatio, cameraComponent.nearPlane, cameraComponent.farPlane);
		}
		else
		{
			sceneData.camera.projectionMatrix = Float4x4::Ortho(sceneData.camera.resolution.x, sceneData.camera.resolution.y, cameraComponent.nearPlane, cameraComponent.farPlane);
		}

		sceneData.camera.viewProjectionMatrix = sceneData.camera.projectionMatrix * sceneData.camera.viewMatrix;
		sceneData.camera.invViewMatrix = Math::Inverse(sceneData.camera.viewMatrix);
		sceneData.camera.invProjectionMatrix = Math::Inverse(sceneData.camera.projectionMatrix);
		sceneData.camera.invViewProjectionMatrix = Math::Inverse(sceneData.camera.viewProjectionMatrix);
		sceneData.camera.position = cameraEntity.GetWorldPosition();

        for (auto renderer : mRenderers)
        {
            renderer->AddRenderPasses(graph, blackboard);
        }
        graph.Compile();

		auto frameIdx = mSwapchain->GetFrameIndex();
		const auto cmd = mCommandBuffers[frameIdx].get();

		TStringStream cmdBufferName;
		cmdBufferName << "Scene CommandBuffer[" << frameIdx << "]";
		cmd->Begin(cmdBufferName.str());

		mCopyCommandBuffer->Execute();
		mCopyCommandBuffer->WaitUntilCompleted();
		mCopyCommandBuffer->Barrier(cmd);

        graph.Execute(cmd, sceneData);
		mDevice->Dispose(renderGraphContext.allocator, sceneTarget);

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
	mSwapchainSize = mSwapchain->GetCurrentDrawable().GetDescriptor().size;
}

CopyCommandBuffer* RenderSystem::GetCopyCommandBuffer()
{
	return mCopyCommandBuffer.get();
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

GPUAllocator* RenderSystem::GetAllocator()
{
	return mPersistentAllocator.get();
}

const GPUAllocator* RenderSystem::GetAllocator() const
{
	return mPersistentAllocator.get();
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
					if (shader.GetStage() == ShaderStage::Compute)
					{
						for (auto& [handle, pipeline] : mDevice->mComputePipelineCache)
						{
							if (handle == pipelineHash)
							{
								auto newPipeline = mDevice->CompileComputePipeline(pipeline.GetDescriptor());
								if (newPipeline.IsValid())
								{
									mDevice->Dispose(pipeline);
									pipeline = newPipeline;
								}
								break;
							}
						}
					}
					else if (shader.GetStage() == ShaderStage::Vertex || shader.GetStage() == ShaderStage::Fragment)
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
			}
			break;
		}
	}
}
