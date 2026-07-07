#pragma once
#include "Core/Subsystem.h"
#include "Math/Size.h"
#include "World/Entity.h"
#include "Shaders/ShaderTypes.h"

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

GENUM(MeshShadingPath, "1CBD20EB-4FAC-4B6C-B45A-191788F36D5E", Serializable, PrettyName("Mesh Shading Path"))
{
	GITEM(Visibility, "4EF1BFFE-62FC-43A8-9782-6356D7EC1D07", PrettyName("Visibility")),
	GITEM(Forward, "0BE8FE10-807D-47B3-B91F-CEE44E01BCCC", PrettyName("Forward"))
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

	MeshShadingPath GetMeshShadingPath() const;
    
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

	void RegisterShadingPipelines(const Material* material);
    
private:

	CameraRenderData SetupCameraRenderData(RenderGraph& graph, const Entity& entity) const;

	SkyAtmosphereRenderData SetupSkyAtmosphereRenderData(RenderGraph& graph, const Entity& entity) const;

	void InitializeBackend();

	bool mRendererResized = false;

	Size mSwapchainSize = {};

	SkyAtmosphereParameters mAtmosphereParams = {};
	EntityHandle mSkyAtmosphereEntity = InvalidEntity;
	EntityHandle mActiveCamera = InvalidEntity;
	EntityHandle mPrevCamera = InvalidEntity;

	float4x4 mPrevCameraView = {};
	float4x4 mPrevCameraViewProjection = {};

	Engine* mEngine;
	
	RenderPath mRenderPath = RenderPath::Default;
	MeshShadingPath mMeshShadingPath = MeshShadingPath::Visibility;

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

