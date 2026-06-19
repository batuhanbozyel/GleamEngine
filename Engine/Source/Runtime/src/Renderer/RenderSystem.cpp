//
//  RenderSystem.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 24.06.2023.
//

#include "gpch.h"
#include "RenderSystem.h"
#include "Swapchain.h"
#include "CommandBuffer.h"
#include "GraphicsDevice.h"
#include "RayTracingScene.h"
#include "CopyCommandBuffer.h"
#include "RenderPipeline.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBlackboard.h"

#include "Renderers/PathTracer.h"
#include "Renderers/BRDFRenderer.h"
#include "Renderers/DepthPrepass.h"
#include "Renderers/SkyAtmosphere.h"
#include "Renderers/WorldRenderer.h"
#include "Renderers/SunShadowRenderer.h"
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

	mPersistentAllocator = new GPUAllocator(mDevice, GPUAllocatorDescriptor{ .name = "Persistent GPU Allocator" });
	mTransientAllocator = new GPUAllocator(mDevice, GPUAllocatorDescriptor{ .name = "Transient GPU Allocator" });
	mCopyCommandBuffer = new CopyCommandBuffer(mDevice);
	mRayTracingScene = new RayTracingScene(mDevice, mTransientAllocator);

	RenderContext context = GetRenderContext();
	{
		auto brdfRenderer = new BRDFRenderer();
		brdfRenderer->OnCreate(context);
		mSharedRenderers.push_back(brdfRenderer);

		auto depthPrepass = new DepthPrepass();
		depthPrepass->OnCreate(context);
		mSharedRenderers.push_back(depthPrepass);

		auto postProcessStack = new PostProcessStack();
		postProcessStack->OnCreate(context);
		mSharedRenderers.push_back(postProcessStack);

		mRenderPipelines[(uint32_t)RenderPath::Default] = new RenderPipeline(context);
		mRenderPipelines[(uint32_t)RenderPath::Default]->AddSharedRenderer(brdfRenderer);
		mRenderPipelines[(uint32_t)RenderPath::Default]->AddRenderer<ReflectionProbeRenderer>();
		mRenderPipelines[(uint32_t)RenderPath::Default]->AddSharedRenderer(depthPrepass);
		mRenderPipelines[(uint32_t)RenderPath::Default]->AddRenderer<SunShadowRenderer>();
		mRenderPipelines[(uint32_t)RenderPath::Default]->AddRenderer<WorldRenderer>();
		mRenderPipelines[(uint32_t)RenderPath::Default]->AddRenderer<SkyAtmosphereRenderer>();
		mRenderPipelines[(uint32_t)RenderPath::Default]->AddSharedRenderer(postProcessStack);
		
		mRenderPipelines[(uint32_t)RenderPath::PathTracing] = new RenderPipeline(context);
		mRenderPipelines[(uint32_t)RenderPath::PathTracing]->AddSharedRenderer(brdfRenderer);
		mRenderPipelines[(uint32_t)RenderPath::PathTracing]->AddSharedRenderer(depthPrepass);
		mRenderPipelines[(uint32_t)RenderPath::PathTracing]->AddRenderer<PathTracer>();
		mRenderPipelines[(uint32_t)RenderPath::PathTracing]->AddSharedRenderer(postProcessStack);
	}

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
	delete mRayTracingScene;
	delete mCopyCommandBuffer;

	for (auto cmd : mCommandBuffers)
	{
		delete cmd;
	}
	mCommandBuffers.clear();

	auto context = GetRenderContext();
	for (auto renderer : mSharedRenderers)
	{
		renderer->OnDestroy(context);
		delete renderer;
	}

	for (auto pipeline : mRenderPipelines)
	{
		delete pipeline;
	}
	mReleaseQueue->Clear();

	delete mTransientAllocator;
	delete mPersistentAllocator;

	delete mReleaseQueue;
	delete mDevice;
	delete mSwapchain;
}

void RenderSystem::PreRender(const World* world)
{
	auto sceneProxy = world->GetSubsystem<RenderSceneProxy>();
	const auto& globalInstances = sceneProxy->GetGlobalInstances();
	if (not globalInstances.empty())
	{
		mCopyCommandBuffer->Commit(sceneProxy->GetGlobalInstanceBuffer(), globalInstances.data(), sizeof(MeshInstanceData) * globalInstances.size(), 0);
	}

	// update active camera
	mPrevCamera = mActiveCamera;
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
	const auto cmd = mCommandBuffers[frameIdx];

	cmd->WaitUntilCompleted();
	if (mRendererResized)
	{
		mSwapchain->Resize(mDevice, mSwapchainSize);
		mRendererResized = false;
	}

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
		renderGraphContext.device = mDevice;
		renderGraphContext.surface = mSwapchain;
		renderGraphContext.allocator = mTransientAllocator;
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
		if (mPrevCamera != mActiveCamera)
		{
			mPrevCameraView = sceneData.camera.uniforms.viewMatrix;
			mPrevCameraViewProjection = sceneData.camera.uniforms.viewProjectionMatrix;

			sceneData.camera.uniforms.prevViewMatrix = mPrevCameraView;
			sceneData.camera.uniforms.prevViewProjectionMatrix = mPrevCameraViewProjection;
		}

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
		const auto cmd = mCommandBuffers[frameIdx];

		TStringStream cmdBufferName;
		cmdBufferName << "Scene CommandBuffer[" << frameIdx << "]";
		cmd->Begin(cmdBufferName.str());

		mCopyCommandBuffer->Execute();
		mCopyCommandBuffer->WaitUntilCompleted();
		mCopyCommandBuffer->Barrier(cmd);

		sceneData.accelerationStructure = mRayTracingScene->BuildAccelerationStructure(cmd, sceneData.sceneProxy);

        graph.Execute(cmd, sceneData);
		mDevice->Dispose(renderGraphContext.allocator, sceneTarget, BarrierStage::None);

		mRayTracingScene->ReleaseAccelerationStructure();

		mSwapchain->Present(cmd);

		mPrevCameraView = sceneData.camera.uniforms.viewMatrix;
		mPrevCameraViewProjection = sceneData.camera.uniforms.viewProjectionMatrix;
    }
}

void RenderSystem::Configure(const RendererConfig& config)
{
	mEngine->UpdateConfig(config);
    mDevice->Configure(config);

    mCommandBuffers.resize(mSwapchain->GetFramesInFlight());
	for (auto& cmd : mCommandBuffers)
	{
		cmd = new CommandBuffer(mDevice);
	}
	mSwapchainSize = mSwapchain->GetCurrentDrawable().GetDescriptor().size;
}

void RenderSystem::SetRenderPath(RenderPath path)
{
	mRenderPath = path;
}

RenderPath RenderSystem::GetRenderPath() const
{
	return mRenderPath;
}

GraphicsDevice* RenderSystem::GetDevice()
{
    return mDevice;
}

const GraphicsDevice* RenderSystem::GetDevice() const
{
    return mDevice;
}

RenderSurface* RenderSystem::GetSurface()
{
	return mSwapchain;
}

const RenderSurface* RenderSystem::GetSurface() const
{
	return mSwapchain;
}

CopyCommandBuffer* RenderSystem::GetCopyCommandBuffer()
{
	return mCopyCommandBuffer;
}

const CopyCommandBuffer* RenderSystem::GetCopyCommandBuffer() const
{
	return mCopyCommandBuffer;
}

RayTracingScene* RenderSystem::GetRayTracingScene()
{
	return mRayTracingScene;
}

const RayTracingScene* RenderSystem::GetRayTracingScene() const
{
	return mRayTracingScene;
}

RenderPipeline* RenderSystem::GetRenderPipeline(RenderPath renderPath)
{
	return mRenderPipelines[(uint32_t)renderPath];
}

const RenderPipeline* RenderSystem::GetRenderPipeline(RenderPath renderPath) const
{
	return mRenderPipelines[(uint32_t)renderPath];
}

RenderPipeline* RenderSystem::GetActiveRenderPipeline()
{
	return mRenderPipelines[(uint32_t)mRenderPath];
}

const RenderPipeline* RenderSystem::GetActiveRenderPipeline() const
{
	return mRenderPipelines[(uint32_t)mRenderPath];
}

GPUAllocator* RenderSystem::GetAllocator()
{
	return mPersistentAllocator;
}

const GPUAllocator* RenderSystem::GetAllocator() const
{
	return mPersistentAllocator;
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
					else if (shader.GetStage() == ShaderStage::RayGeneration ||
							 shader.GetStage() == ShaderStage::Miss ||
							 shader.GetStage() == ShaderStage::ClosestHit ||
							 shader.GetStage() == ShaderStage::AnyHit ||
							 shader.GetStage() == ShaderStage::Intersection)
					{
						for (auto& [handle, pipeline] : mDevice->mRayTracingPipelineCache)
						{
							if (handle == pipelineHash)
							{
								auto newPipeline = mDevice->CompileRayTracingPipeline(pipeline.GetDescriptor());
								if (newPipeline.IsValid())
								{
									mDevice->Dispose(pipeline);
									pipeline = newPipeline;
								}
								break;
							}
						}
					}
					else if (shader.GetStage() == ShaderStage::Mesh || shader.GetStage() == ShaderStage::Amplification)
					{
						for (auto& [handle, pipeline] : mDevice->mMeshPipelineCache)
						{
							if (handle == pipelineHash)
							{
								auto newPipeline = mDevice->CompileMeshPipeline(pipeline.GetDescriptor());
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

void RenderSystem::RegisterShadingPipelines(const Material* material)
{
	mRayTracingScene->RegisterShadingPipeline(material);
	for (auto pipeline : mRenderPipelines)
	{
		for (auto renderer : *pipeline)
		{
			renderer->RegisterShadingPipeline(material);
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
	camera.uniforms.prevViewMatrix = mPrevCameraView;
	camera.uniforms.prevViewProjectionMatrix = mPrevCameraViewProjection;
	camera.uniforms.invViewMatrix = Math::Inverse(camera.uniforms.viewMatrix);
	camera.uniforms.invProjectionMatrix = Math::Inverse(camera.uniforms.projectionMatrix);
	camera.uniforms.invViewProjectionMatrix = Math::Inverse(camera.uniforms.viewProjectionMatrix);
	camera.uniforms.position = entity.GetWorldPosition();
	camera.uniforms.nearPlane = cameraComponent.nearPlane;
	camera.uniforms.farPlane = cameraComponent.farPlane;
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

RenderContext RenderSystem::GetRenderContext() const
{
	RenderContext context;
	context.device = mDevice;
	context.surface = mSwapchain;
	context.releaseQueue = mReleaseQueue;
	context.allocator = mPersistentAllocator;
	return context;
}
