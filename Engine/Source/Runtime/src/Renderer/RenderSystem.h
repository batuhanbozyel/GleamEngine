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

	void SetCameraOverride(EntityHandle camera);

	uint32_t GetFrameCount() const;
    
    GraphicsDevice* GetDevice();
    
    const GraphicsDevice* GetDevice() const;

	RenderSurface* GetSurface();

	const RenderSurface* GetSurface() const;

	CopyCommandBuffer* GetCopyCommandBuffer();

	const CopyCommandBuffer* GetCopyCommandBuffer() const;

	RayTracingScene* GetRayTracingScene();

	const RayTracingScene* GetRayTracingScene() const;
	
	void AddRenderer(IRenderer* renderer, RenderStage stage);

	void RemoveRenderer(IRenderer* renderer);

	template<RendererType T>
	T* GetRenderer() const
	{
		return mRenderers.Find<T>();
	}

	IRenderer* AcquireRenderer(const Reflection::ClassDescription& classDesc);

	void ReleaseRenderer(IRenderer* renderer);

	template<RendererType T>
	T* GetSharedRenderer()
	{
		return static_cast<T*>(AcquireRenderer(Reflection::GetClass<T>()));
	}

	void RegisterPipeline(RenderPipeline* pipeline);

	void UnregisterPipeline(RenderPipeline* pipeline);

	const AssetReference& GetDefaultRenderPipeline() const;

	EntityHandle GetActiveCamera() const;

	GPUAllocator* GetAllocator();

	const GPUAllocator* GetAllocator() const;

	void RecompileShader(const TString& entryPoint);

	RenderContext GetRenderContext() const;

	void RegisterShadingPipelines(const Material* material);
    
private:

	CameraRenderData SetupCameraRenderData(RenderGraph& graph, const Entity& entity) const;

	SkyAtmosphereRenderData SetupSkyAtmosphereRenderData(RenderGraph& graph, const Entity& entity);

	void ApplyGraphicsSettings(RenderPipeline* pipeline, const GraphicsSettings& settings) const;

	void DestroyRenderer(IRenderer* renderer);

	void InitializeBackend();

	uint32_t mFrameCount = 0;

	bool mRendererResized = false;

	Size mSwapchainSize = {};

	SkyAtmosphereParameters mAtmosphereParams = {};
	EntityHandle mSkyAtmosphereEntity = InvalidEntity;
	EntityHandle mActiveCamera = InvalidEntity;
	EntityHandle mPrevCamera = InvalidEntity;
	EntityHandle mCameraOverride = InvalidEntity;

	float4x4 mPrevCameraView = {};
	float4x4 mPrevCameraViewProjection = {};

	struct RendererAllocation
	{
		void* memory = nullptr;
		const TypeLifecycle* lifecycle = nullptr;
	};

	RendererList mRenderers;
	HashMap<IRenderer*, RendererAllocation> mRendererAllocations;
	HashMap<uint32_t, IRenderer*> mSharedRenderers;
	TArray<RenderPipeline*> mPipelines;
	AssetReference mDefaultRenderPipeline;

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

