#pragma once
#include "Core/Subsystem.h"
#include "Math/Size.h"
#include "World/Entity.h"
#include "Shaders/ShaderTypes.h"
#include "MeshDrawList.h"
#include "RenderPipeline.h"
#include "Assets/AssetReference.h"

namespace Gleam {

class World;
class Material;
class Swapchain;
class IRenderer;
class RenderGraph;
class GPUAllocator;
class CommandBuffer;
class RenderSurface;
class SceneView;
class GraphicsDevice;
class RayTracingScene;
class CopyCommandBuffer;
class ResourceReleaseQueue;

struct RenderContext;
struct RendererConfig;
struct GraphicsSettings;
struct CameraRenderData;
struct SkyAtmosphereRenderData;

class TypeLifecycle;

class RenderSystem final : public EngineSubsystem
{
public:
    
    virtual void Initialize(Engine* engine) override;
    
    virtual void Shutdown(Engine* engine) override;

	void PreRender(const World* world);
    
    void Render(const World* world);
    
    void Configure(const RendererConfig& config);

	void ApplyGraphicsSettings(const GraphicsSettings& settings);

	uint32_t GetFrameCount() const;
    
    GraphicsDevice* GetDevice();
    
    const GraphicsDevice* GetDevice() const;

	RenderSurface* GetSurface();

	const RenderSurface* GetSurface() const;

	CopyCommandBuffer* GetCopyCommandBuffer();

	const CopyCommandBuffer* GetCopyCommandBuffer() const;

	RayTracingScene* GetRayTracingScene();

	const RayTracingScene* GetRayTracingScene() const;
	
	void AddFrameRenderer(IRenderer* renderer, RenderStage stage);

	void RemoveFrameRenderer(IRenderer* renderer);

	IRenderer* AcquireRenderer(const Reflection::ClassDescription& classDesc);

	void ReleaseRenderer(IRenderer* renderer);

	template<RendererType T>
	T* GetSharedRenderer()
	{
		return static_cast<T*>(AcquireRenderer(Reflection::GetClass<T>()));
	}

	void RegisterPipeline(RenderPipeline* pipeline);

	void UnregisterPipeline(RenderPipeline* pipeline);

	GPUAllocator* GetAllocator();

	const GPUAllocator* GetAllocator() const;

	SceneView* AcquireSceneView(const World* world, EntityHandle camera);

	void ReleaseSceneView(const World* world, EntityHandle camera);

	void ReleaseSceneViews(const World* world);

	SceneView* GetSceneView(const World* world, EntityHandle camera);

	void RecompileShader(const TString& entryPoint);

	RenderContext GetRenderContext() const;

	void RegisterShadingPipelines(const Material* material);
    
private:

	CameraRenderData SetupCameraRenderData(const Entity& entity, const SceneView& view, const Size& resolution) const;

	SkyAtmosphereRenderData SetupSkyAtmosphereRenderData(const Entity& entity);

	void AddSceneTargetPass(RenderGraph& graph, SceneRenderingData& sceneData, const Size& resolution) const;

	void AddTargetTransitionPass(RenderGraph& graph, const TextureHandle& target) const;

	bool IsSharedRenderer(const IRenderer* renderer) const;

	void ApplyGraphicsSettings(RenderPipeline* pipeline, const GraphicsSettings& settings) const;

	void DestroyRenderer(IRenderer* renderer);

	void DestroySceneView(SceneView* view);

	void InitializeBackend();

	uint32_t mFrameCount = 0;

	bool mRendererResized = false;

	Size mSwapchainSize = {};

	EntityHandle mSkyAtmosphereEntity = InvalidEntity;
	SceneView* mMainView = nullptr;
	TArray<SceneView*> mTargetViews;

	struct RendererAllocation
	{
		void* memory = nullptr;
		const TypeLifecycle* lifecycle = nullptr;
	};

	RendererList mFrameRenderers;
	HashMap<IRenderer*, RendererAllocation> mRendererAllocations;
	HashMap<uint32_t, IRenderer*> mSharedRenderers;
	TArray<RenderPipeline*> mPipelines;
	AssetReference mDefaultRenderPipeline;
	TArray<SceneView*> mSceneViews;

	RayTracingScene* mRayTracingScene = nullptr;

    Swapchain* mSwapchain;

	GraphicsDevice* mDevice = nullptr;

	CopyCommandBuffer* mCopyCommandBuffer = nullptr;

	ResourceReleaseQueue* mReleaseQueue = nullptr;

	GPUAllocator* mTransientAllocator = nullptr;

	GPUAllocator* mPersistentAllocator = nullptr;
    
	TArray<CommandBuffer*> mCommandBuffers = {};
    
};

} // namespace Gleam

