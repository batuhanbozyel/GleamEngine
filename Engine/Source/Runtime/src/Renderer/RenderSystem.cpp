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
#include "SceneView.h"
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
#include "Renderers/VisibilityClassification.h"
#include "Renderers/GBufferResolveRenderer.h"
#include "Renderers/AmbientOcclusionRenderer.h"
#include "Renderers/RayTracedReflectionRenderer.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Application.h"
#include "Core/ConfigSystem.h"
#include "Assets/AssetManager.h"
#include "Core/TypeLifecycle.h"
#include "Container/Pointer.h"
#include "Core/Events/RendererEvent.h"
#include "GraphicsSettings.h"

#include "World/World.h"
#include "World/Systems/PickingSystem.h"
#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void RenderSystem::Initialize(Engine* engine)
{
	InitializeBackend();

	auto configSystem = engine->GetSubsystem<ConfigSystem>();
	const auto& renderConfig = configSystem->Register<RendererConfig>();
	configSystem->Subscribe<RendererConfig>([this](const RendererConfig& config)
	{
		Configure(config);
	});
	Configure(renderConfig);

	GLEAM_ASSERT(mDevice->GetFeatures().meshShaders, "Mesh shaders is not supported");

	const auto& graphicsSettings = configSystem->Register<GraphicsSettings>();
	configSystem->Subscribe<GraphicsSettings>([this](const GraphicsSettings& settings)
	{
		ApplyGraphicsSettings(settings);
	});
	ApplyGraphicsSettings(graphicsSettings);

	EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
	{
		auto size = Size((float)e.GetWidth(), (float)e.GetHeight());
		if (mSwapchainSize != size)
		{
			mRendererResized = true;
			mSwapchainSize = size;
			EventDispatcher<RendererResizeEvent>::Publish(RendererResizeEvent(size));
		}
	});
}

void RenderSystem::Shutdown(Engine* engine)
{
	GLEAM_ASSERT(mSceneViews.empty(), "Scene views must be released before render system shutdown");
	delete mRayTracingScene;
	delete mCopyCommandBuffer;

	for (auto cmd : mCommandBuffers)
	{
		delete cmd;
	}
	mCommandBuffers.clear();

	for (const auto& [_, renderer] : mSharedRenderers)
	{
		DestroyRenderer(renderer);
	}
	mSharedRenderers.clear();
	mReleaseQueue->Clear();

	delete mTransientAllocator;
	delete mPersistentAllocator;

	delete mReleaseQueue;
	delete mDevice;
	delete mSwapchain;
}

void RenderSystem::PreRender(const World* world)
{
	++mFrameCount;

	mActiveCamera = InvalidEntity;
	mSkyAtmosphereEntity = InvalidEntity;

	if (world)
	{
		// update active camera
		const auto& entityManager = world->GetEntityManager();
		if (entityManager.IsValid(mCameraOverride) and entityManager.HasComponent<Camera>(mCameraOverride))
		{
			mActiveCamera = mCameraOverride;
		}
		else
		{
			entityManager.ForEach<Entity, Camera>([&](const Entity& entity, const Camera& component)
			{
				if (entity.IsActive())
				{
					mActiveCamera = entity;
				}
			});
		}

		entityManager.ForEach<Entity, SkyAtmosphere>([&](const Entity& entity, const SkyAtmosphere& component)
		{
			if (entity.IsActive())
			{
				mSkyAtmosphereEntity = entity;
			}
		});

		auto sceneProxy = world->GetSubsystem<RenderSceneProxy>();
		sceneProxy->Update(world, mActiveCamera);
	}

	auto frameIdx = mSwapchain->GetFrameIndex();
	const auto cmd = mCommandBuffers[frameIdx];

	cmd->WaitUntilCompleted();
	if (mRendererResized)
	{
		mSwapchain->Resize(mDevice, mSwapchainSize);
		mRendererResized = false;
	}

	mReleaseQueue->BeginFrame();
	mTransientAllocator->CollectGarbage(mSwapchain->GetFramesInFlight() + 1);
	mPersistentAllocator->CollectGarbage(mSwapchain->GetFramesInFlight() + 1);

	if (world and world->HasSubsystem<PickingSystem>())
	{
		world->GetSubsystem<PickingSystem>()->ResolveReadback(frameIdx);
	}
}

void RenderSystem::Render(const World* world)
{
#ifdef USE_METAL_RENDERER
	@autoreleasepool
#endif
	{
		SceneView* view = world and mActiveCamera != InvalidEntity ? GetSceneView(world, mActiveCamera) : nullptr;
		const RenderPipeline* pipeline = view ? view->pipeline : nullptr;
		const bool hasScene = pipeline != nullptr;

		RenderGraphContext renderGraphContext;
		renderGraphContext.device = mDevice;
		renderGraphContext.surface = mSwapchain;
		renderGraphContext.allocator = mTransientAllocator;
		RenderGraph graph(renderGraphContext);

		RenderGraphBlackboard blackboard;
		auto& sceneData = blackboard.Add<SceneRenderingData>();
		sceneData.backbuffer = graph.ImportBackbuffer();

		Texture sceneTarget;
		MeshDrawList cameraDrawList;
		RenderSceneProxy* sceneProxy = nullptr;
		if (hasScene)
		{
			const auto& cameraComponent = world->GetEntityManager().GetComponent<Camera>(mActiveCamera);
			const auto& cameraEntity = world->GetEntityManager().GetComponent<Entity>(mActiveCamera);
			sceneProxy = world->GetSubsystem<RenderSceneProxy>();

			// TODO: Render scene per active camera
			// Set sceneTarget to camera target
			// Camera target default requires a special handle
			// which maps to backbuffer for runtime and a temporary texture for editor scene view
			// We may want to decide this on Editor side
			RenderTextureDescriptor sceneTargetDesc{};
			sceneTargetDesc.name = "Scene Target";
			sceneTargetDesc.size = cameraComponent.GetViewport();
			sceneTargetDesc.format = mSwapchain->GetFormat();

			sceneTarget = mDevice->CreateTexture(renderGraphContext.allocator, sceneTargetDesc);
			sceneData.sceneTarget = graph.ImportTexture(sceneTarget);
			sceneData.sceneProxy = sceneProxy;
			sceneData.world = world;
			sceneData.view = view;

			// Setup camera & sky atmosphere
			Entity atmosphereEntity = mSkyAtmosphereEntity != InvalidEntity ? world->GetEntityManager().GetComponent<Entity>(mSkyAtmosphereEntity) : Entity();
			sceneData.atmosphere = SetupSkyAtmosphereRenderData(graph, atmosphereEntity);
			sceneData.camera = SetupCameraRenderData(graph, cameraEntity, *view);

			sceneProxy->Cull(sceneData.camera.uniforms.frustum, cameraDrawList);
			sceneData.drawList = &cameraDrawList;

			if (not view->historyValid)
			{
				sceneData.camera.uniforms.prevViewMatrix = sceneData.camera.uniforms.viewMatrix;
				sceneData.camera.uniforms.prevViewProjectionMatrix = sceneData.camera.uniforms.viewProjectionMatrix;
			}

			if (memcmp(&mAtmosphereParams, &sceneData.atmosphere.params, sizeof(SkyAtmosphereParameters)) != 0)
			{
				mAtmosphereParams = sceneData.atmosphere.params;
				GetSharedRenderer<SkyAtmosphereRenderer>()->UpdateSkyAtmosphere(graph, blackboard);
			}
		}

		auto renderers = hasScene ? pipeline->GetRenderers() : RendererList();
		renderers.Insert(mRenderers);
		for (const auto& entry : renderers)
		{
			entry.renderer->AddRenderPasses(graph, blackboard);
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

		if (hasScene)
		{
			sceneProxy->BuildInstanceBuffer(cmd, mTransientAllocator);
			sceneData.accelerationStructure = mRayTracingScene->BuildAccelerationStructure(cmd, sceneData.sceneProxy);
		}

        graph.Execute(cmd);

		if (hasScene)
		{
			mDevice->Dispose(renderGraphContext.allocator, sceneTarget, BarrierStage::None);
			sceneProxy->ReleaseInstanceBuffer(mTransientAllocator);
			mRayTracingScene->ReleaseAccelerationStructure();

			view->prevViewMatrix = sceneData.camera.uniforms.viewMatrix;
			view->prevViewProjectionMatrix = sceneData.camera.uniforms.viewProjectionMatrix;
			view->historyValid = true;
		}

		for (auto sceneView : mSceneViews)
		{
			if (sceneView != view or not hasScene)
			{
				sceneView->ReleaseHistory(mDevice, mTransientAllocator);
			}
		}

		mSwapchain->Present(cmd);
    }
}

void RenderSystem::Configure(const RendererConfig& config)
{
    mDevice->Configure(config);

    mCommandBuffers.resize(mSwapchain->GetFramesInFlight());
	for (auto& cmd : mCommandBuffers)
	{
		cmd = new CommandBuffer(mDevice, mTransientAllocator);
	}
	mSwapchainSize = mSwapchain->GetCurrentDrawable().GetDescriptor().size;
}

void RenderSystem::ApplyGraphicsSettings(const GraphicsSettings& settings)
{
	mDefaultRenderPipeline = settings.renderPipeline;
	for (auto pipeline : mPipelines)
	{
		ApplyGraphicsSettings(pipeline, settings);
	}
}

// User pipelines may leave any of these renderers out
void RenderSystem::ApplyGraphicsSettings(RenderPipeline* pipeline, const GraphicsSettings& settings) const
{
	if (auto renderer = pipeline->GetRenderer<SunShadowRenderer>())
	{
		renderer->SetSettings(settings.shadows);
	}
	if (auto renderer = pipeline->GetRenderer<AmbientOcclusionRenderer>())
	{
		renderer->SetSettings(settings.ambientOcclusion);
	}
	if (auto renderer = pipeline->GetRenderer<RayTracedReflectionRenderer>())
	{
		renderer->SetSettings(settings.rayTracedReflections);
	}
}

void RenderSystem::AddRenderer(IRenderer* renderer, RenderStage stage)
{
	mRenderers.Insert(renderer, stage);
}

void RenderSystem::RemoveRenderer(IRenderer* renderer)
{
	mRenderers.Remove(renderer);
}

static size_t RendererBaseOffset(const Reflection::ClassDescription& classDesc)
{
	for (const auto& base : classDesc.ResolveBaseClasses())
	{
		if (base.TypeHash() == Reflection::TypeHash<IRenderer>())
		{
			return base.GetOffset();
		}
	}
	GLEAM_ASSERT(false, "{0} does not derive from IRenderer", classDesc.ResolveName());
	return 0;
}

IRenderer* RenderSystem::AcquireRenderer(const Reflection::ClassDescription& classDesc)
{
	const bool shared = classDesc.GetAttribute<Reflection::Attribute::Renderer>()->shared;
	if (shared)
	{
		if (auto it = mSharedRenderers.find(classDesc.TypeHash()); it != mSharedRenderers.end())
		{
			return it->second;
		}
	}

	RendererAllocation allocation;
	allocation.lifecycle = TypeLifecycleRegistry::GetInstance()->Find(classDesc.TypeHash());
	allocation.memory = ::operator new(allocation.lifecycle->GetSize(), std::align_val_t(allocation.lifecycle->GetAlignment()));
	allocation.lifecycle->Construct(allocation.memory);

	auto renderer = static_cast<IRenderer*>(OffsetPointer(allocation.memory, RendererBaseOffset(classDesc)));
	renderer->OnCreate(GetRenderContext());
	mRendererAllocations.emplace(renderer, allocation);
	if (shared)
	{
		mSharedRenderers.emplace(classDesc.TypeHash(), renderer);
	}
	return renderer;
}

void RenderSystem::ReleaseRenderer(IRenderer* renderer)
{
	const bool shared = eastl::any_of(mSharedRenderers.begin(), mSharedRenderers.end(), [renderer](const auto& pair)
	{
		return pair.second == renderer;
	});
	if (shared == false)
	{
		DestroyRenderer(renderer);
	}
}

void RenderSystem::DestroyRenderer(IRenderer* renderer)
{
	const auto it = mRendererAllocations.find(renderer);
	renderer->OnDestroy(GetRenderContext());
	it->second.lifecycle->Destruct(it->second.memory);
	::operator delete(it->second.memory, std::align_val_t(it->second.lifecycle->GetAlignment()));
	mRendererAllocations.erase(it);
}

void RenderSystem::RegisterPipeline(RenderPipeline* pipeline)
{
	mPipelines.push_back(pipeline);
	ApplyGraphicsSettings(pipeline, Globals::Engine->GetSubsystem<ConfigSystem>()->Get<GraphicsSettings>());
}

void RenderSystem::UnregisterPipeline(RenderPipeline* pipeline)
{
	mPipelines.erase(eastl::find(mPipelines.begin(), mPipelines.end(), pipeline));
}

const AssetReference& RenderSystem::GetDefaultRenderPipeline() const
{
	return mDefaultRenderPipeline;
}

EntityHandle RenderSystem::GetActiveCamera() const
{
	return mActiveCamera;
}

void RenderSystem::SetCameraOverride(EntityHandle camera)
{
	mCameraOverride = camera;
}

uint32_t RenderSystem::GetFrameCount() const
{
	return mFrameCount;
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

GPUAllocator* RenderSystem::GetAllocator()
{
	return mPersistentAllocator;
}

SceneView* RenderSystem::AcquireSceneView(const World* world, EntityHandle camera)
{
	auto view = GetSceneView(world, camera);
	if (view == nullptr)
	{
		view = new SceneView(world, camera);
		mSceneViews.push_back(view);
	}
	return view;
}

void RenderSystem::ReleaseSceneView(const World* world, EntityHandle camera)
{
	auto it = eastl::find_if(mSceneViews.begin(), mSceneViews.end(), [world, camera](const SceneView* view)
	{
		return view->world == world and view->camera == camera;
	});
	if (it != mSceneViews.end())
	{
		DestroySceneView(*it);
		*it = mSceneViews.back();
		mSceneViews.pop_back();
	}
}

void RenderSystem::ReleaseSceneViews(const World* world)
{
	for (uint32_t i = 0; i < mSceneViews.size();)
	{
		if (mSceneViews[i]->world == world)
		{
			DestroySceneView(mSceneViews[i]);
			mSceneViews[i] = mSceneViews.back();
			mSceneViews.pop_back();
		}
		else
		{
			++i;
		}
	}
}

SceneView* RenderSystem::GetSceneView(const World* world, EntityHandle camera)
{
	auto it = eastl::find_if(mSceneViews.begin(), mSceneViews.end(), [world, camera](const SceneView* view)
	{
		return view->world == world and view->camera == camera;
	});
	return it != mSceneViews.end() ? *it : nullptr;
}

void RenderSystem::DestroySceneView(SceneView* view)
{
	Globals::GameInstance->GetSubsystem<AssetManager>()->Release(view->pipelineReference);
	view->ReleaseHistory(mDevice, mTransientAllocator);
	delete view;
}

const GPUAllocator* RenderSystem::GetAllocator() const
{
	return mPersistentAllocator;
}

void RenderSystem::RecompileShader(const TString& entryPoint)
{
	mCommandBuffers[mSwapchain->GetFrameIndex()]->WaitUntilCompleted();

	if (mDevice->RecompileNativeShader(entryPoint))
	{
		return;
	}

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
	for (auto pipeline : mPipelines)
	{
		pipeline->RegisterShadingPipeline(material);
	}
	for (const auto& entry : mRenderers)
	{
		entry.renderer->RegisterShadingPipeline(material);
	}
}

CameraRenderData RenderSystem::SetupCameraRenderData(RenderGraph& graph, const Entity& entity, const SceneView& view) const
{
	CameraRenderData camera = {};
	camera.entity = entity;

	const auto& cameraComponent = entity.GetComponent<Camera>();
	camera.uniforms.resolution = cameraComponent.GetViewport();
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
	camera.uniforms.prevViewMatrix = view.prevViewMatrix;
	camera.uniforms.prevViewProjectionMatrix = view.prevViewProjectionMatrix;
	camera.uniforms.invViewMatrix = Math::Inverse(camera.uniforms.viewMatrix);
	camera.uniforms.invProjectionMatrix = Math::Inverse(camera.uniforms.projectionMatrix);
	camera.uniforms.invViewProjectionMatrix = Math::Inverse(camera.uniforms.viewProjectionMatrix);
	camera.uniforms.position = entity.GetWorldPosition();
	camera.uniforms.nearPlane = cameraComponent.nearPlane;
	camera.uniforms.farPlane = cameraComponent.farPlane;

	// Gribb-Hartmann frustum plane extraction. Shaders see the view-projection matrix transposed
	// (row-major CPU storage interpreted column-major by HLSL), so transpose here to match the rows
	// the GPU would read.
	const Float4x4 vp = Math::Transpose(camera.uniforms.viewProjectionMatrix);
	camera.uniforms.frustum.planes[0] = Math::Normalize(Plane(vp.row[3] + vp.row[0])); // left
	camera.uniforms.frustum.planes[1] = Math::Normalize(Plane(vp.row[3] - vp.row[0])); // right
	camera.uniforms.frustum.planes[2] = Math::Normalize(Plane(vp.row[3] + vp.row[1])); // bottom
	camera.uniforms.frustum.planes[3] = Math::Normalize(Plane(vp.row[3] - vp.row[1])); // top
	camera.uniforms.frustum.planes[4] = Math::Normalize(Plane(vp.row[2]));             // near
	camera.uniforms.frustum.planes[5] = Math::Normalize(Plane(vp.row[3] - vp.row[2])); // far
	return camera;
}

SkyAtmosphereRenderData RenderSystem::SetupSkyAtmosphereRenderData(RenderGraph& graph, const Entity& entity)
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

		auto skyAtmosphereRenderer = GetSharedRenderer<SkyAtmosphereRenderer>();
		const ImportResourceParams lutParams{ .initialLayout = mSkyAtmosphereLutLayout, .finalLayout = BarrierLayout::ShaderResource };
		skyAtmosphere.transmittanceLut = graph.ImportTexture(skyAtmosphereRenderer->GetTransmittanceLutTexture(), lutParams);
		skyAtmosphere.multiScatterLut = graph.ImportTexture(skyAtmosphereRenderer->GetMultiScatterLutTexture(), lutParams);
		mSkyAtmosphereLutLayout = BarrierLayout::ShaderResource;
		skyAtmosphere.params = skyAtmosphereRenderer->GetSkyAtmosphereParameters(atmosphereComponent.atmosphere);
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
