//
//  Renderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once
#include "Math/Rect.h"
#include "Buffer.h"
#include "Shader.h"
#include "Texture.h"
#include "Pipeline.h"
#include "RendererConfig.h"
#include "AccelerationStructure.h"
#include "RenderPassDescriptor.h"
#include "Shaders/ShaderTypes.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBlackboard.h"
#include "World/Entity.h"

namespace Gleam {

class World;
class Material;
class SceneView;
class RenderSystem;
class Swapchain;
class RenderPipeline;
class GraphicsDevice;
class RenderSceneProxy;
struct MeshDrawList;
class ResourceReleaseQueue;

struct RenderContext
{
	ResourceReleaseQueue* releaseQueue = nullptr;
	GPUAllocator* allocator = nullptr;
	GraphicsDevice* device = nullptr;
	Swapchain* surface = nullptr;
};

struct CameraRenderData
{
	CameraUniforms uniforms = {};
	EntityHandle entity = InvalidEntity;
};

struct SkyAtmosphereRenderData
{
	SkyAtmosphereUniforms uniforms = {};
	SkyAtmosphereParameters params = {};
	TextureHandle transmittanceLut = TextureHandle();
	TextureHandle multiScatterLut = TextureHandle();
	EntityHandle entity = InvalidEntity;
};

struct SceneRenderingData
{
    const RenderSceneProxy* sceneProxy = nullptr;
    const MeshDrawList* drawList = nullptr;
    const World* world = nullptr;
	SceneView* view = nullptr;
	CameraRenderData camera = {};
	SkyAtmosphereRenderData atmosphere = {};
    TextureHandle backbuffer = TextureHandle();
	Rect viewport = Rect();
	TextureHandle sceneTarget = TextureHandle();
	AccelerationStructureView accelerationStructure = AccelerationStructureView();
};

GENUM(RenderStage, "80F31B84-37F1-479C-A004-F1C4244EEAD7", PrettyName("Render Stage"))
{
	GITEM(BeforeRendering, "BDEADD45-0E70-42B8-9775-43A0183211BE", PrettyName("Before Rendering")),
	GITEM(Prepass, "F36ABAAC-4E31-4BDF-8903-032DC144738C"),
	GITEM(Shadows, "DF2DCA4D-63E0-409B-8499-F5FA881747E7"),
	GITEM(Opaque, "76E43A64-D9AC-4951-8E0B-03B37C03EDCF"),
	GITEM(Sky, "2D5CCA26-8AA1-4465-9E3B-68B41FB61D90"),
	GITEM(Transparent, "6DA0D242-CB23-435D-B443-61E7DC0D81AA"),
	GITEM(PostProcess, "1AF614AB-33B3-4F07-9031-7ED654613E56", PrettyName("Post Process")),
	GITEM(AfterRendering, "8658251F-EFF7-4FA6-922E-62D7EEC3D681", PrettyName("After Rendering"))
};

} // namespace Gleam

namespace Gleam::Reflection::Attribute {

GLEAM_ATTRIBUTE(Renderer)
{
	RenderStage stage = RenderStage::Opaque;

	bool shared = false;

	explicit constexpr Renderer(RenderStage stage, bool shared = false)
		: stage(stage)
		, shared(shared)
	{

	}
};

} // namespace Gleam::Reflection::Attribute

namespace Gleam {

GCLASS(IRenderer, "483BC11D-9D2B-495C-B38D-8215254011C1")
{
public:
    
    friend class RenderSystem;
	friend class RenderPipeline;
    friend class GraphicsDevice;
    
    virtual ~IRenderer() = default;

protected:

	virtual void OnCreate(const RenderContext& context) {}

	virtual void OnDestroy(const RenderContext& context) {}

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) = 0;

	virtual void RegisterShadingPipeline(const Material* material) {}

};

} // namespace Gleam
