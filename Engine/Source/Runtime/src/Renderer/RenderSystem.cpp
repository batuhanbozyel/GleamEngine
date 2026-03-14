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

#include "Renderers/PathTracer.h"
#include "Renderers/SkyAtmosphere.h"
#include "Renderers/WorldRenderer.h"
#include "Renderers/PostProcessStack.h"
#include "Renderers/ReflectionProbeRenderer.h"

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
	Configure(engine->GetConfiguration().renderer);

	RenderContext context;
	context.device = mDevice.get();
	context.surface = mSwapchain.get();
	context.releaseQueue = mReleaseQueue.get();
	context.allocator = mPersistentAllocator.get();
	
	mRenderPipelines[(uint32_t)RenderPath::Default] = CreateScope<RenderPipeline>(context);
	mRenderPipelines[(uint32_t)RenderPath::Default]->AddRenderer<ReflectionProbeRenderer>();
	mRenderPipelines[(uint32_t)RenderPath::Default]->AddRenderer<WorldRenderer>();
	mRenderPipelines[(uint32_t)RenderPath::Default]->AddRenderer<SkyAtmosphereRenderer>();
	mRenderPipelines[(uint32_t)RenderPath::Default]->AddRenderer<PostProcessStack>();
	
	mRenderPipelines[(uint32_t)RenderPath::PathTracing] = CreateScope<RenderPipeline>(context);
	mRenderPipelines[(uint32_t)RenderPath::PathTracing]->AddRenderer<PathTracer>();
	mRenderPipelines[(uint32_t)RenderPath::PathTracing]->AddRenderer<PostProcessStack>();

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
	mRenderPath = RenderPath::Default;
}

void RenderSystem::Shutdown(Engine* engine)
{
	mCopyCommandBuffer.reset();
	mCommandBuffers.clear();

	for (auto& pipeline : mRenderPipelines)
	{
		pipeline.reset();
	}

	mTransientAllocator.reset();
	mPersistentAllocator.reset();

	mReleaseQueue.reset();
    mDevice.reset();
	mSwapchain.reset();
}

void RenderSystem::PreRender(const World* world)
{
	auto sceneProxy = world->GetSubsystem<RenderSceneProxy>();
	sceneProxy->ForEach([&](const MeshBatch& batch)
	{
		mCopyCommandBuffer->Commit(batch.instanceBuffer, batch.instances.data(), sizeof(MeshInstanceData) * batch.numInstances, 0);
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

	mSkyAtmosphereEntity = InvalidEntity;
	world->GetEntityManager().ForEach<Entity, SkyAtmosphere>([&](const Entity& entity, const SkyAtmosphere& component)
	{
		if (entity.IsActive())
		{
			mSkyAtmosphereEntity = entity;
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
		sceneData.backbuffer = graph.ImportTexture(backbuffer);
		sceneData.sceneTarget = graph.ImportTexture(sceneTarget);
		sceneData.sceneProxy = world->GetSubsystem<RenderSceneProxy>();
		sceneData.world = world;

		// Setup camera & sky atmosphere
		Entity atmosphereEntity = mSkyAtmosphereEntity != InvalidEntity ? world->GetEntityManager().GetComponent<Entity>(mSkyAtmosphereEntity) : Entity();
		sceneData.atmosphere = SetupSkyAtmosphereRenderData(graph, atmosphereEntity);
		sceneData.camera = SetupCameraRenderData(graph, cameraEntity);

		auto defaultPipeline = GetRenderPipeline(RenderPath::Default);
		if (auto skyAtmosphereRenderer = defaultPipeline->GetRenderer<SkyAtmosphereRenderer>(); skyAtmosphereRenderer)
		{
			if (memcmp(&mAtmosphereParams, &sceneData.atmosphere.params, sizeof(SkyAtmosphereParameters)) != 0)
			{
				mAtmosphereParams = sceneData.atmosphere.params;
				skyAtmosphereRenderer->UpdateSkyAtmosphere(graph, blackboard);
			}
		}

		auto pipeline = GetActiveRenderPipeline();
        for (auto renderer : *pipeline)
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

void RenderSystem::SetRenderPath(RenderPath path)
{
	mRenderPath = path;
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

RenderPipeline* RenderSystem::GetRenderPipeline(RenderPath renderPath)
{
	return mRenderPipelines[(uint32_t)renderPath].get();
}

const RenderPipeline* RenderSystem::GetRenderPipeline(RenderPath renderPath) const
{
	return mRenderPipelines[(uint32_t)renderPath].get();
}

RenderPipeline* RenderSystem::GetActiveRenderPipeline()
{
	return mRenderPipelines[(uint32_t)mRenderPath].get();
}

const RenderPipeline* RenderSystem::GetActiveRenderPipeline() const
{
	return mRenderPipelines[(uint32_t)mRenderPath].get();
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

CameraRenderData RenderSystem::SetupCameraRenderData(RenderGraph& graph, const Entity& entity) const
{
	CameraRenderData camera = {};
	camera.entity = entity;

	const auto& cameraComponent = entity.GetComponent<Camera>();
	camera.uniforms.resolution = Float2(cameraComponent.orthographicSize * cameraComponent.aspectRatio, cameraComponent.orthographicSize);
	camera.uniforms.viewMatrix = Float4x4::LookTo(entity.GetWorldPosition(), entity.ForwardVector(), entity.UpVector());
	if (cameraComponent.projectionType == ProjectionType::Perspective)
	{
		camera.uniforms.projectionMatrix = Float4x4::Perspective(Math::Deg2Rad(cameraComponent.fov), cameraComponent.aspectRatio, cameraComponent.nearPlane, cameraComponent.farPlane);
	}
	else
	{
		camera.uniforms.projectionMatrix = Float4x4::Ortho(camera.uniforms.resolution.x, camera.uniforms.resolution.y, cameraComponent.nearPlane, cameraComponent.farPlane);
	}
	camera.uniforms.viewProjectionMatrix = camera.uniforms.projectionMatrix * camera.uniforms.viewMatrix;
	camera.uniforms.invViewMatrix = Math::Inverse(camera.uniforms.viewMatrix);
	camera.uniforms.invProjectionMatrix = Math::Inverse(camera.uniforms.projectionMatrix);
	camera.uniforms.invViewProjectionMatrix = Math::Inverse(camera.uniforms.viewProjectionMatrix);
	camera.uniforms.position = entity.GetWorldPosition();
	return camera;
}

SkyAtmosphereRenderData RenderSystem::SetupSkyAtmosphereRenderData(RenderGraph& graph, const Entity& entity) const
{
	SkyAtmosphereRenderData skyAtmosphere = {};
	skyAtmosphere.entity = entity;

	skyAtmosphere.uniforms.sunIlluminance = 1.0f;
	skyAtmosphere.uniforms.sunDirection = Float3::up;
	if (entity.IsValid())
	{
		const auto& atmosphereComponent = entity.GetComponent<SkyAtmosphere>();
		skyAtmosphere.uniforms.sunIlluminance = Float3(atmosphereComponent.sun.color.r, atmosphereComponent.sun.color.g, atmosphereComponent.sun.color.b) * atmosphereComponent.sun.intensity;
		skyAtmosphere.uniforms.sunAngularDiameter = atmosphereComponent.sun.angularDiameter;
		skyAtmosphere.uniforms.sunDirection = entity.UpVector();

		auto pipeline = GetRenderPipeline(RenderPath::Default);
		auto skyAtmosphereRenderer = pipeline->GetRenderer<SkyAtmosphereRenderer>();
		if (skyAtmosphereRenderer)
		{
			skyAtmosphere.transmittanceLut = graph.ImportTexture(skyAtmosphereRenderer->GetTransmittanceLutTexture());
			skyAtmosphere.multiScatterLut = graph.ImportTexture(skyAtmosphereRenderer->GetMultiScatterLutTexture());
			skyAtmosphere.params = skyAtmosphereRenderer->GetSkyAtmosphereParameters(atmosphereComponent.atmosphere);
		}
	}
	skyAtmosphere.uniforms.transmittanceLutTexture = skyAtmosphere.transmittanceLut;
	skyAtmosphere.uniforms.multiScatterLutTexture = skyAtmosphere.multiScatterLut;
	return skyAtmosphere;
}
