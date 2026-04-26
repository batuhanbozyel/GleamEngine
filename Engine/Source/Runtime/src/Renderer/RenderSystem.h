#pragma once
#include "Core/Subsystem.h"
#include "Math/Size.h"
#include "World/Entity.h"
#include "Shaders/ShaderTypes.h"

namespace Gleam {

class World;
class Swapchain;
class IRenderer;
class RenderGraph;
class GPUAllocator;
class CommandBuffer;
class RenderSurface;
class GraphicsDevice;
class RenderPipeline;
class RayTracingScene;
class CopyCommandBuffer;
class ResourceReleaseQueue;

struct RenderContext;
struct RendererConfig;
struct CameraRenderData;
struct SkyAtmosphereRenderData;

GENUM(RenderPath, "83FD8433-7D91-42AF-A237-CCF726E306E5", Serializable, PrettyName("Render Path"))
{
	GITEM(Default, "E77464D0-418E-48E9-87AA-2C7355C7C599", PrettyName("Default")),
	GITEM(PathTracing, "AF1EDBC0-9EA0-4C5B-9BDB-8258B6CCE31E", PrettyName("Path Tracing"))
};

class RenderSystem final : public EngineSubsystem
{
public:
    
    virtual void Initialize(Engine* engine) override;
    
    virtual void Shutdown(Engine* engine) override;

	void PreRender(const World* world);
    
    void Render(const World* world);
    
    void Configure(const RendererConfig& config);

	void SetRenderPath(RenderPath path);

	RenderPath GetRenderPath() const;
    
    GraphicsDevice* GetDevice();
    
    const GraphicsDevice* GetDevice() const;

	RenderSurface* GetSurface();

	const RenderSurface* GetSurface() const;

	CopyCommandBuffer* GetCopyCommandBuffer();

	const CopyCommandBuffer* GetCopyCommandBuffer() const;

	RayTracingScene* GetRayTracingScene();

	const RayTracingScene* GetRayTracingScene() const;
	
	RenderPipeline* GetRenderPipeline(RenderPath renderPath);
	
	const RenderPipeline* GetRenderPipeline(RenderPath renderPath) const;

	RenderPipeline* GetActiveRenderPipeline();

	const RenderPipeline* GetActiveRenderPipeline() const;

	GPUAllocator* GetAllocator();

	const GPUAllocator* GetAllocator() const;

	void RecompileShader(const TString& entryPoint);

	RenderContext GetRenderContext() const;
    
private:

	CameraRenderData SetupCameraRenderData(RenderGraph& graph, const Entity& entity) const;

	SkyAtmosphereRenderData SetupSkyAtmosphereRenderData(RenderGraph& graph, const Entity& entity) const;

	void InitializeBackend();

	bool mRendererResized = false;

	Size mSwapchainSize = {};

	SkyAtmosphereParameters mAtmosphereParams = {};
	EntityHandle mSkyAtmosphereEntity = InvalidEntity;
	EntityHandle mActiveCamera = InvalidEntity;

	Engine* mEngine;
	
	RenderPath mRenderPath = RenderPath::Default;
	TArray<RenderPipeline*, 2> mRenderPipelines = {};
	TArray<IRenderer*> mSharedRenderers = {};

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

