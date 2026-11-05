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
#include "RenderTexture.h"
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
#include "World/Systems/CameraSystem.h"
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

	mSkyAtmosphereEntity = InvalidEntity;

	if (world)
	{
		const auto& entityManager = world->GetEntityManager();
		entityManager.ForEach<Entity, SkyAtmosphere>([&](const Entity& entity, const SkyAtmosphere& component)
		{
			if (entity.IsActive())
			{
				mSkyAtmosphereEntity = entity;
			}
		});
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
		RenderGraphContext renderGraphContext;
		renderGraphContext.device = mDevice;
		renderGraphContext.surface = mSwapchain;
		renderGraphContext.allocator = mTransientAllocator;
		RenderGraph graph(renderGraphContext);

		RenderGraphBlackboard frameBlackboard;
		auto& frameData = frameBlackboard.Add<SceneRenderingData>();
		frameData.backbuffer = graph.ImportBackbuffer({ .clearColor = Color::black, .clearOnFirstUse = true });

		RenderSceneProxy* sceneProxy = nullptr;
		TArray<SceneView*> views;
		if (world)
		{
			sceneProxy = world->GetSubsystem<RenderSceneProxy>();
			sceneProxy->Update(world);

			views = GatherViews(world, sceneProxy);

			frameData.sceneProxy = sceneProxy;
			frameData.world = world;
		}

		struct ViewRenderData
		{
			RenderGraphBlackboard blackboard;
			MeshDrawList drawList;
		};
		// Pass lambdas keep references into these until Execute, so the storage must not reallocate
		TArray<ViewRenderData> viewData;
		viewData.reserve(views.size());

		if (world and not views.empty())
		{
			const auto& entityManager = world->GetEntityManager();
			Entity atmosphereEntity = mSkyAtmosphereEntity != InvalidEntity ? entityManager.GetComponent<Entity>(mSkyAtmosphereEntity) : Entity();
			frameData.atmosphere = SetupSkyAtmosphereRenderData(atmosphereEntity);
			GetSharedRenderer<SkyAtmosphereLutRenderer>()->AddRenderPasses(graph, frameBlackboard);

			RendererList sharedRenderers;
			for (auto view : views)
			{
				for (const auto& entry : view->GetRenderPipeline()->GetRenderers())
				{
					const bool added = eastl::any_of(sharedRenderers.begin(), sharedRenderers.end(), [&entry](const auto& shared)
					{
						return shared.renderer == entry.renderer;
					});
					if (added == false and IsSharedRenderer(entry.renderer))
					{
						sharedRenderers.Insert(entry.renderer, entry.stage);
					}
				}
			}
			for (const auto& entry : sharedRenderers)
			{
				entry.renderer->AddRenderPasses(graph, frameBlackboard);
			}

			// Views sharing a RenderTexture share one import, so only the first of them clears it
			TArray<eastl::pair<const RenderTexture*, TextureHandle>> targets;
			for (uint32_t viewIndex = 0; viewIndex < views.size(); ++viewIndex)
			{
				const auto view = views[viewIndex];
				auto& data = viewData.emplace_back();
				data.blackboard = frameBlackboard;

				auto& sceneData = data.blackboard.Get<SceneRenderingData>();
				sceneData.view = view;
				if (view->GetRenderTexture())
				{
					auto target = eastl::find_if(targets.begin(), targets.end(), [view](const auto& entry)
					{
						return entry.first == view->GetRenderTexture();
					});
					if (target == targets.end())
					{
						const auto renderTexture = view->GetRenderTexture();
						targets.emplace_back(renderTexture, graph.ImportTexture(renderTexture->GetTexture(), { .clearColor = renderTexture->GetClearColor(), .finalLayout = BarrierLayout::ShaderResource, .clearOnFirstUse = renderTexture->ShouldClear() }));
						target = targets.end() - 1;
					}
					sceneData.backbuffer = target->second;
				}

				sceneData.viewport = view->GetRect();
				const Size resolution = sceneData.viewport.size;
				AddSceneTargetPass(graph, sceneData, resolution);

				const auto& cameraEntity = entityManager.GetComponent<Entity>(view->GetCameraEntity());
				sceneData.camera = SetupCameraRenderData(cameraEntity, *view, resolution);
				if (not view->historyValid)
				{
					sceneData.camera.uniforms.prevViewMatrix = sceneData.camera.uniforms.viewMatrix;
					sceneData.camera.uniforms.prevViewProjectionMatrix = sceneData.camera.uniforms.viewProjectionMatrix;
				}

				sceneProxy->Cull(sceneData.camera.uniforms.frustum, data.drawList);
				sceneData.drawList = &data.drawList;

				// Pipeline renderers go in first, so attached renderers of the same stage run after them
				RendererList renderers;
				for (const auto& entry : view->GetRenderPipeline()->GetRenderers())
				{
					if (not IsSharedRenderer(entry.renderer))
					{
						renderers.Insert(entry.renderer, entry.stage);
					}
				}
				renderers.Insert(view->GetRenderers());
				for (const auto& entry : renderers)
				{
					entry.renderer->AddRenderPasses(graph, data.blackboard);
				}

				// Later views may sample this target, so it is transitioned once its last writer is done
				const bool lastWriter = view->GetRenderTexture() and eastl::none_of(views.begin() + viewIndex + 1, views.end(), [view](const SceneView* other)
				{
					return other->GetRenderTexture() == view->GetRenderTexture();
				});
				if (lastWriter)
				{
					AddTargetTransitionPass(graph, sceneData.backbuffer);
				}
			}
		}

		for (const auto& entry : mFrameRenderers)
		{
			entry.renderer->AddRenderPasses(graph, frameBlackboard);
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

		if (sceneProxy)
		{
			sceneProxy->BuildInstanceBuffer(cmd, mTransientAllocator);
			const auto accelerationStructure = mRayTracingScene->BuildAccelerationStructure(cmd, sceneProxy);
			for (auto& data : viewData)
			{
				data.blackboard.Get<SceneRenderingData>().accelerationStructure = accelerationStructure;
			}
		}

        graph.Execute(cmd);

		if (sceneProxy)
		{
			sceneProxy->ReleaseInstanceBuffer(mTransientAllocator);
			mRayTracingScene->ReleaseAccelerationStructure();
		}

		for (auto& data : viewData)
		{
			const auto& sceneData = data.blackboard.Get<SceneRenderingData>();
			sceneData.view->prevViewMatrix = sceneData.camera.uniforms.viewMatrix;
			sceneData.view->prevViewProjectionMatrix = sceneData.camera.uniforms.viewProjectionMatrix;
			sceneData.view->historyValid = true;
		}

		for (auto sceneView : mSceneViews)
		{
			if (eastl::find(views.begin(), views.end(), sceneView) == views.end())
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

	for (auto view : mSceneViews)
	{
		AcquireSceneView(view->GetWorld(), view->GetCameraEntity());
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

void RenderSystem::AddFrameRenderer(IRenderer* renderer, RenderStage stage)
{
	mFrameRenderers.Insert(renderer, stage);
}

void RenderSystem::RemoveFrameRenderer(IRenderer* renderer)
{
	mFrameRenderers.Remove(renderer);
}

bool RenderSystem::IsSharedRenderer(const IRenderer* renderer) const
{
	return eastl::any_of(mSharedRenderers.begin(), mSharedRenderers.end(), [renderer](const auto& pair)
	{
		return pair.second == renderer;
	});
}

TArray<SceneView*> RenderSystem::GatherViews(const World* world, const RenderSceneProxy* sceneProxy)
{
	TArray<SceneView*> views;
	if (sceneProxy->GetMainCamera() == InvalidEntity)
	{
		return views;
	}

	const auto& sceneViews = sceneProxy->GetViews();
	for (auto view : sceneViews.targetViews)
	{
		if (view->GetRenderTexture() and view->ShouldRender())
		{
			views.push_back(view);
		}
	}

	// Swapchain cameras only render while CameraSystem runs
	if (world->GetSystem<CameraSystem>()->Enabled)
	{
		for (auto view : sceneViews.swapchainViews)
		{
			if (view->ShouldRender())
			{
				views.push_back(view);
			}
		}
	}
	return views;
}

void RenderSystem::AddSceneTargetPass(RenderGraph& graph, SceneRenderingData& sceneData, const Size& resolution) const
{
	struct SceneTargetPassData
	{
		TextureHandle sceneTarget;
	};

	// The graph only allocates a transient inside a pass that writes it, so the target is created and cleared here
	graph.AddRenderPass<SceneTargetPassData>("RenderSystem::SceneTarget", [&](RenderGraphBuilder& builder, SceneTargetPassData& passData)
	{
		RenderTextureDescriptor descriptor;
		descriptor.name = "SceneTarget";
		descriptor.size = resolution;
		descriptor.format = TextureFormat::R16G16B16A16_SFloat;
		descriptor.clearBuffer = true;
		passData.sceneTarget = builder.UseColorBuffer(builder.CreateTexture(descriptor));
		sceneData.sceneTarget = passData.sceneTarget;
	},
	[](const CommandBuffer* cmd, const SceneTargetPassData& passData)
	{
	});
}

// Bindless readers (ImGui, materials of later views) don't declare graph reads, so the target leaves its view as a shader resource
void RenderSystem::AddTargetTransitionPass(RenderGraph& graph, const TextureHandle& target) const
{
	struct TargetTransitionPassData
	{
		TextureHandle target;
	};

	graph.AddRenderPass<TargetTransitionPassData>("RenderSystem::TargetTransition", [&](RenderGraphBuilder& builder, TargetTransitionPassData& passData)
	{
		passData.target = builder.ReadTexture(target);
	},
	[](const CommandBuffer* cmd, const TargetTransitionPassData& passData)
	{
	});
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
	if (IsSharedRenderer(renderer) == false)
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

Swapchain* RenderSystem::GetSurface()
{
	return mSwapchain;
}

const Swapchain* RenderSystem::GetSurface() const
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
		view = new SceneView(mSwapchain, world, camera);
		mSceneViews.push_back(view);
	}
	else
	{
		RendererList renderers = view->GetRenderers();
		view->ReleaseHistory(mDevice, mTransientAllocator);
		view->~SceneView();
		new (view) SceneView(mSwapchain, world, camera);
		for (const auto& entry : renderers)
		{
			view->AddRenderer(entry.renderer, entry.stage);
		}
	}
	return view;
}

void RenderSystem::ReleaseSceneView(const World* world, EntityHandle camera)
{
	auto it = eastl::find_if(mSceneViews.begin(), mSceneViews.end(), [world, camera](const SceneView* view)
	{
		return view->GetWorld() == world and view->GetCameraEntity() == camera;
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
		if (mSceneViews[i]->GetWorld() == world)
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
		return view->GetWorld() == world and view->GetCameraEntity() == camera;
	});
	return it != mSceneViews.end() ? *it : nullptr;
}

void RenderSystem::DestroySceneView(SceneView* view)
{
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
}

CameraRenderData RenderSystem::SetupCameraRenderData(const Entity& entity, const SceneView& view, const Size& resolution) const
{
	CameraRenderData camera = {};
	camera.entity = entity;

	const auto& cameraComponent = entity.GetComponent<Camera>();
	camera.uniforms.resolution = resolution;
	camera.uniforms.viewMatrix = Float4x4::LookTo(entity.GetWorldPosition(), entity.ForwardVector(), entity.UpVector());
	camera.uniforms.projectionMatrix = cameraComponent.GetProjectionMatrix(resolution.width / resolution.height);
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

SkyAtmosphereRenderData RenderSystem::SetupSkyAtmosphereRenderData(const Entity& entity)
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
		skyAtmosphere.params = GetSharedRenderer<SkyAtmosphereLutRenderer>()->GetSkyAtmosphereParameters(atmosphereComponent.atmosphere);
	}
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

AssetReference RenderSystem::GetDefaultRenderPipeline() const
{
	return mDefaultRenderPipeline;
}
