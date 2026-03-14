#pragma once
#include "Core/Subsystem.h"
#include "Swapchain.h"
#include "CommandBuffer.h"
#include "CopyCommandBuffer.h"
#include "GraphicsDevice.h"
#include "ResourceReleaseQueue.h"
#include "RenderPipeline.h"
#include "World/Entity.h"

namespace Gleam {

class World;
class CopyCommandBuffer;

struct CameraRenderData;
struct SkyAtmosphereRenderData;

GENUM(RenderPath, "83FD8433-7D91-42AF-A237-CCF726E306E5", Serializable)
{
	GITEM(Default, "E77464D0-418E-48E9-87AA-2C7355C7C599"),
	GITEM(PathTracing, "AF1EDBC0-9EA0-4C5B-9BDB-8258B6CCE31E")
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

	CopyCommandBuffer* GetCopyCommandBuffer();
    
    GraphicsDevice* GetDevice();
    
    const GraphicsDevice* GetDevice() const;

	RenderSurface* GetSurface();

	const RenderSurface* GetSurface() const;
	
	RenderPipeline* GetRenderPipeline(RenderPath renderPath);
	
	const RenderPipeline* GetRenderPipeline(RenderPath renderPath) const;

	RenderPipeline* GetActiveRenderPipeline();

	const RenderPipeline* GetActiveRenderPipeline() const;

	GPUAllocator* GetAllocator();

	const GPUAllocator* GetAllocator() const;

	void RecompileShader(const TString& entryPoint);
    
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
	
	RenderPath mRenderPath;
	TArray<Scope<RenderPipeline>, 2> mRenderPipelines;

    Scope<Swapchain> mSwapchain;

	Scope<GraphicsDevice> mDevice;

	Scope<CopyCommandBuffer> mCopyCommandBuffer;

	Scope<ResourceReleaseQueue> mReleaseQueue;

	Scope<GPUAllocator> mTransientAllocator;

	Scope<GPUAllocator> mPersistentAllocator;
    
    TArray<Scope<CommandBuffer>> mCommandBuffers;
    
};

} // namespace Gleam

