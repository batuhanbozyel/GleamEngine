# Renderer

The renderer is a bindless, render-graph driven renderer with a DirectX 12 backend (Windows) and a Metal backend (macOS). Both backends use a single HLSL shader codebase. Code lives in `Engine/Source/Runtime/src/Renderer/`.

## Table of Contents
* [Overview](#overview)
* [Frame Flow](#frame-flow)
* [RHI](#rhi)
* [Bindless Resources](#bindless-resources)
* [Shaders](#shaders)
* [Pipelines](#pipelines)
* [Command Buffers](#command-buffers)
* [Render Graph](#render-graph)
* [Renderers and Render Pipelines](#renderers-and-render-pipelines)
* [Scene Data](#scene-data)
* [Materials](#materials)
* [Writing a Renderer](#writing-a-renderer)

## Overview

| Layer | Types | Role |
| --- | --- | --- |
| Engine subsystem | `RenderSystem` | Owns the device, swapchain, allocators and render pipelines; renders the active world every frame |
| Render pipelines | `RenderPipeline`, `IRenderer` | An ordered list of renderers per `RenderPath` (`Default`, `PathTracing`) |
| Render graph | `RenderGraph`, `RenderGraphBuilder`, `RenderGraphBlackboard` | Per-frame pass graph: transient resources, ordering, barriers |
| RHI | `GraphicsDevice`, `CommandBuffer`, `Buffer`, `Texture`, pipelines | Thin wrapper over D3D12 / Metal, resolved at compile time |
| Scene | `RenderSceneProxy`, `RayTracingScene`, `MeshDrawList` | GPU-side mirror of the world's meshes, culling, acceleration structures |
| Shaders | `Shaders/*.hlsl`, `ShaderTypes.h`, `ShaderInterop.h` | HLSL shared with C++ |

## Frame Flow

`Application::Run` calls `RenderSystem::PreRender(world)` then `RenderSystem::Render(world)` after the world update.

**PreRender**

1. Picks the active camera: the camera override if set (`SetCameraOverride`, used by the editor viewport), otherwise the last active entity with a `Camera`.
2. Picks the last active `SkyAtmosphere` entity.
3. Calls `RenderSceneProxy::Update`, which syncs mesh records from `MeshRenderer` changes.
4. Waits for this frame slot's command buffer, handles a pending swapchain resize, and garbage-collects allocators and the release queue.

**Render**

1. Creates a fresh `RenderGraph` and `RenderGraphBlackboard`, and adds `SceneRenderingData` with the imported backbuffer.
2. If there is a world and a camera: creates the scene target at the camera's viewport size, fills camera and atmosphere uniforms, and frustum-culls into a `MeshDrawList`.
3. Calls `AddRenderPasses` on every renderer of the active render pipeline, in order, then calls `graph.Compile()`.
4. Begins the command buffer, flushes the copy command buffer (uploads), builds the instance buffer and the TLAS.
5. Calls `graph.Execute(cmd, sceneData)`, releases per-frame resources, and presents.

## RHI

`GraphicsDevice` creates and disposes GPU objects; `CommandBuffer` records commands. Neither uses virtual dispatch: each backend implements the same functions in `DirectX/` or `Metal/`, selected at compile time (`USE_DIRECTX_RENDERER` / `USE_METAL_RENDERER`). Backend differences stay inside the backend, so callers write the same code on both platforms.

```cpp
auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
GraphicsDevice* device = renderSystem->GetDevice();
GPUAllocator* allocator = renderSystem->GetAllocator();

Buffer buffer = device->CreateBuffer(allocator, BufferDescriptor{ .name = "Particles", .size = sizeof(Particle) * count });
TextureDescriptor textureDesc;
textureDesc.name = "Noise";
textureDesc.size = Size{ 256.0f, 256.0f };
textureDesc.format = TextureFormat::R8G8B8A8_UNorm;
Texture texture = device->CreateTexture(allocator, textureDesc);

device->Dispose(allocator, buffer, BarrierStage::None);
device->Dispose(allocator, texture, BarrierStage::None);
```

- Disposal is deferred through the `ResourceReleaseQueue` until the GPU no longer uses the object.
- Uploads go through `RenderSystem::GetCopyCommandBuffer()`, which is flushed at the start of each frame's command buffer.
- On DirectX the backend uses Enhanced Barriers. Barriers between render graph passes are generated automatically; explicit `cmd->Barrier(BarrierGroup)` is only needed outside the graph.
- `GetFeatures()` reports device capabilities (ray tracing, mesh shaders).

## Bindless Resources

There is no per-draw resource binding. Every buffer and texture gets a slot in a global descriptor heap, and shaders index `ResourceDescriptorHeap` directly.

| C++ / HLSL type | Meaning |
| --- | --- |
| `ShaderResourceIndex` | Read-only view (SRV) index; `InvalidResourceIndex` when unset |
| `UnorderedAccessIndex` | Read-write view (UAV) index |
| `Texture2DResourceView<T>` | Typed texture view with `Load`, `Sample`, `SampleLevel`, `SampleGrad`, `IsValid` |

Get indices from resources:

```cpp
ShaderResourceIndex srv = buffer.GetResourceView();
UnorderedAccessIndex uav = texture.GetUnorderedAccessView();         // mip 0
UnorderedAccessIndex mipUav = texture.GetUnorderedAccessView(2);     // a specific mip
```

Inside the render graph, a `TextureHandle` or `BufferHandle` converts to `ShaderResourceIndex` implicitly.

### Shared types

Structs passed to shaders are declared once in `Shaders/ShaderTypes.h` and compiled by both C++ and HLSL. `ShaderInterop.h` maps `float4`, `float4x4`, `uint2`... to the engine math types in C++.

```cpp
// Shaders/ShaderTypes.h
struct BlurConstants
{
	ShaderResourceIndex source;
	UnorderedAccessIndex destination;
	uint2 resolution;
	float radius;
};
```

- Mirror C++ enums for HLSL under `#ifndef __cplusplus`, pass them as `uint32_t`, and cast in the shader.
- When adding a field to a shared struct, fill it at every construction site, even where it is unused.

## Shaders

All shaders are HLSL in `Runtime/src/Renderer/Shaders/`. Each entry point is marked with a `[shader("...")]` attribute and referenced from C++ **by its entry-point name**.

```hlsl
#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::BlurConstants, constants);

[shader("compute")]
[numthreads(8, 8, 1)]
void blurComputeShader(uint2 id : SV_DispatchThreadID)
{
    Texture2D<float4> source = ResourceDescriptorHeap[constants.source];
    RWTexture2D<float4> destination = ResourceDescriptorHeap[constants.destination];
    destination[id] = source.Load(int3(id, 0));
}
```

- **Build step.** The `RuntimeShaders` CMake target runs `ShaderCompilerCLI` over the shader folder and writes one DXIL file per entry point into `Assets/Shaders/<entryPoint>.dxil` (debug info in Debug/Development). Backend-only shaders live in `Shaders/DirectX/` and `Shaders/Metal/` and compile separately.
- **Metal.** The Metal backend loads the same DXIL and converts it with Apple's Metal Shader Converter, so one HLSL source serves both backends.
- **Constants.** `PUSH_CONSTANT(type, name)` declares the per-dispatch constants (at most 128 bytes, set with `cmd->SetPushConstant`). `CONSTANT_BUFFER(type, name, slot)` declares a constant buffer set with `cmd->SetConstantBuffer(value, slot)`.
- **Hot reload.** `RenderSystem::RecompileShader(entryPoint)` reloads an entry point and rebuilds the pipelines that use it. The editor calls it when a material shader changes.

## Pipelines

Pipelines are described with reflected descriptor structs and created through the device. The device caches them by descriptor hash and returns a lightweight handle; creating the same descriptor twice returns the same pipeline.

```cpp
GraphicsPipelineStateDescriptor graphicsDesc;
graphicsDesc.vertexEntry = "fullscreenTriangleVertexShader";
graphicsDesc.fragmentEntry = "tonemappingFragmentShader";
graphicsDesc.colorFormats = { surface->GetFormat() };
GraphicsPipelineHandle tonemap = device->CreateGraphicsPipeline(graphicsDesc);

ComputePipelineHandle blur = device->CreateComputePipeline({ .entryPoint = "blurComputeShader" });

MeshPipelineStateDescriptor meshDesc;
meshDesc.amplificationEntry = "meshAmplificationShader";
meshDesc.meshEntry = "meshMeshletShader";
meshDesc.fragmentEntry = "myFragmentShader";
MeshPipelineHandle mesh = device->CreateMeshPipeline(meshDesc);
```

| Descriptor | Key fields |
| --- | --- |
| `GraphicsPipelineStateDescriptor` | blend/depth/stencil state, cull mode, topology, color/depth formats, `vertexEntry`, `fragmentEntry` |
| `MeshPipelineStateDescriptor` | same render state, `amplificationEntry`, `meshEntry`, `fragmentEntry` |
| `ComputePipelineStateDescriptor` | `entryPoint` |
| `RayTracingPipelineStateDescriptor` | ray generation / miss entries and hit groups (`HitGroupDescriptor`) |

Handles convert to the pipeline object, so they can be passed straight to `cmd->BindGraphicsPipeline(handle)`.

## Command Buffers

`CommandBuffer` records into the render graph's passes. The graph begins and ends render/compute passes for you; inside a pass callback you bind, set constants and draw or dispatch.

| Group | Calls |
| --- | --- |
| Bind | `BindGraphicsPipeline`, `BindMeshPipeline`, `BindComputePipeline`, `BindRayTracingPipeline` |
| State | `SetViewport`, `SetScissorRect`, `SetPushConstant<T>`, `SetConstantBuffer<T>(value, slot)` |
| Draw | `Draw`, `DrawIndexed`, `DrawIndirect`, `DrawIndexedIndirect`, `DispatchMesh`, `DispatchMeshIndirect` |
| Compute / RT | `Dispatch`, `DispatchIndirect`, `DispatchRays`, `DispatchRaysIndirect` |
| Copy | `CopyBuffer`, `ClearBuffer`, `Blit` |

## Render Graph

Each frame, renderers declare passes and the resources each pass reads and writes. The graph then:

- orders passes by their dependencies (read-after-write, write-after-read, write-after-write), breaking ties by the order passes were added;
- allocates transient textures and buffers at first use and frees them after the last pass that references them;
- inserts the barriers each pass needs and sets attachment load/store actions.

Every added pass executes; the graph does not cull passes. `AllowPassCulling(false)` marks a pass as having side effects, so its attachments are always stored even when nothing reads them afterwards. Writing to an imported (non-transient) resource does this automatically.

```cpp
struct BlurPassData
{
	TextureHandle source;
	TextureHandle destination;
};

auto& blurData = graph.AddComputePass<BlurPassData>("Blur", [&](RenderGraphBuilder& builder, BlurPassData& passData)
{
	RenderTextureDescriptor desc = graph.GetDescriptor(worldData.colorTarget);
	desc.name = "Blurred Color";
	passData.destination = builder.WriteTexture(builder.CreateTexture(desc));
	passData.source = builder.ReadTexture(worldData.colorTarget);
},
[this](const CommandBuffer* cmd, const BlurPassData& passData)
{
	BlurConstants constants;
	constants.source = passData.source;
	constants.destination = passData.destination.GetTexture().GetUnorderedAccessView();
	...
	cmd->BindComputePipeline(mBlurPipeline);
	cmd->SetPushConstant(constants);
	cmd->Dispatch(groupsX, groupsY, 1u);
});
```

[Writing a Renderer](#writing-a-renderer) shows the complete version.

| Builder call | Use |
| --- | --- |
| `CreateTexture(RenderTextureDescriptor)` / `CreateBuffer(BufferDescriptor)` | New transient resource, alive only this frame |
| `ReadTexture` / `ReadBuffer` | Shader read in this pass |
| `WriteTexture` / `WriteBuffer` | Shader write (UAV) in this pass; returns the new version |
| `UseColorBuffer(texture)` | Color attachment of a render pass |
| `UseDepthBuffer(texture, DepthAccess)` | Depth attachment, read-only or read-write |
| `AllowPassCulling(false)` | Always store attachments |

- `AddRenderPass` creates a raster pass; `AddComputePass` creates a compute (or ray tracing) pass. Both return the pass data so later passes can read its handles.
- Always store the handle **returned** by `Write*` / `Use*`, and pass that version to readers; the version is how the graph orders passes.
- `graph.ImportTexture(texture, { .clearColor, .clearOnFirstUse })` brings an externally owned texture (backbuffer, persistent history) into the graph.
- `graph.GetDescriptor(handle)` returns a texture's descriptor at setup time, useful for matching sizes and formats.
- `RenderTextureDescriptor` adds `clearColor`, `clearDepth`, `clearStencil` and `clearBuffer` to `TextureDescriptor`.

### Blackboard

The `RenderGraphBlackboard` passes handles between renderers, keyed by type. A renderer publishes its outputs and later renderers read them:

```cpp
blackboard.Add(BlurData{ .blurredColor = blurData.destination });
const auto& blur = blackboard.Get<BlurData>();
if (blackboard.Has<AmbientOcclusionData>()) { ... }
```

Each type can be added once per frame. Engine outputs include `SceneRenderingData`, `BRDFData`, `DepthPrepassData`, `VisibilityClassificationData`, `GBufferData`, `AmbientOcclusionData`, `SunShadowData`, `RayTracedReflectionData`, `ReflectionProbePassData` and `WorldRenderingData`.

## Renderers and Render Pipelines

A renderer implements `IRenderer`:

| Method | When |
| --- | --- |
| `OnCreate(const RenderContext&)` | Once, when added to a pipeline: create pipelines and persistent resources |
| `OnDestroy(const RenderContext&)` | When removed, or on shutdown: dispose them |
| `AddRenderPasses(graph, blackboard)` | Every frame: declare passes |
| `RegisterShadingPipeline(const Material*)` | When a material loads: build material-specific pipelines |
| `GetStage()` | Where the renderer sits in the pipeline (default `Opaque`) |

`RenderContext` carries the `device`, `allocator`, `surface` and `releaseQueue`.

A `RenderPipeline` keeps its renderers sorted by `RenderStage`, and by insertion order within a stage:
`BeforeRendering` → `Prepass` → `Shadows` → `Opaque` → `Sky` → `Transparent` → `PostProcess` → `AfterRendering`.

```cpp
auto pipeline = renderSystem->GetRenderPipeline(RenderPath::Default);
auto blur = pipeline->AddRenderer<BlurRenderer>(4.0f);    // owned: created, OnCreate'd and deleted by the pipeline
pipeline->AddSharedRenderer(mGridRenderer);               // shared: caller owns it, may be in several pipelines
pipeline->GetRenderer<BlurRenderer>();
pipeline->RemoveRenderer<BlurRenderer>();
pipeline->RemoveSharedRenderer(mGridRenderer);

renderSystem->SetRenderPath(RenderPath::PathTracing);
```

### Built-in pipelines

| Stage | `Default` | `PathTracing` |
| --- | --- | --- |
| BeforeRendering | `BRDFRenderer` (BRDF/GGX LUTs), `ReflectionProbeRenderer` | `BRDFRenderer` |
| Prepass | `DepthPrepass` (mesh-shader depth + visibility buffer), `VisibilityClassificationRenderer`, `GBufferResolveRenderer` | `DepthPrepass` |
| Shadows | `AmbientOcclusionRenderer` (XeGTAO), `SunShadowRenderer` (ray traced), `RayTracedReflectionRenderer` (FidelityFX denoiser) | |
| Opaque | `WorldRenderer` (visibility shading + forward pass for blended materials) | `PathTracer` |
| Sky | `SkyAtmosphereRenderer` | |
| PostProcess | `PostProcessStack` (tonemapping) | `PostProcessStack` |

The editor adds its own renderers to these pipelines: grid, selection outline, view modes, `LineRenderer`, `DebugRenderer` and `ImGuiRenderer`.

## Scene Data

`SceneRenderingData` is always on the blackboard. When no world or camera is active, `world` is `nullptr`; scene renderers should return early in that case.

| Field | Content |
| --- | --- |
| `world` | The world being rendered |
| `sceneProxy` | `RenderSceneProxy`: mesh records grouped into per-material batches, plus the instance buffer |
| `drawList` | Camera-culled `MeshDrawList` |
| `camera` | `CameraUniforms` (view/projection, previous-frame matrices, frustum, resolution) and the camera entity |
| `atmosphere` | Sky atmosphere uniforms and LUT handles |
| `backbuffer` / `sceneTarget` | Swapchain image and the camera's color target |
| `accelerationStructure` | TLAS view for ray tracing passes |

Draw the visible meshes per material batch:

```cpp
sceneData.sceneProxy->ForEach([&](const MeshBatch& batch)
{
	sceneData.sceneProxy->ForEachDraw(batch, *sceneData.drawList, [&](uint32_t instanceID, const MeshEntityRecord& record, const MeshInstanceRecord& instance)
	{
		// record.mesh, record.lod, instance.material, instance.submeshIndex
	});
});
```

Up to `MAX_MESH_INSTANCES` (65536) instances are supported.

## Materials

A material (`.mat`, JSON) names a **surface shader**, its alpha mode, cull mode and properties:

```json
{
    "Cull" : "Back",
    "AlphaMode" : "Mask",
    "Properties" : [
        {"BaseColor": "Float4"},
        {"BaseColorTexture": "Texture2D"},
        {"AlphaCutoff": "Float"}
    ],
    "Lighting": "On",
    "SurfaceShader": "SurfaceLit"
}
```

The surface shader (`.shader`) implements `Gleam::SurfaceOutput SurfMain(Gleam::MeshVertexOut IN)` and reads properties through the generated `Material` struct. `Texture2D` properties become `Texture2DResourceView<float4>`, so check `IsValid()` before sampling.

```hlsl
Gleam::SurfaceOutput SurfMain(Gleam::MeshVertexOut IN)
{
    Gleam::SurfaceOutput OUT;
    OUT.albedo = Material.BaseColor * IN.color;
    if (Material.BaseColorTexture.IsValid())
    {
        OUT.albedo *= Material.BaseColorTexture.SampleGrad(Sampler_Anisotropic_Repeat, IN.uv, IN.ddxUV, IN.ddyUV);
    }
    OUT.emission = 0.0;
    OUT.normal = float3(0.0, 0.0, 1.0);
    OUT.metallic = 0.0;
    OUT.roughness = 0.5;
    OUT.occlusion = 1.0;
    OUT.alphaCutoff = Material.AlphaCutoff;
    return OUT;
}
```

When the editor imports the material, it compiles the surface shader into one entry point per pass, named `<SurfaceShader><Variant>`:

| Variant | Used by |
| --- | --- |
| `DepthPrepass` | `DepthPrepass` (masked and blended materials) |
| `GBufferResolve` | `GBufferResolveRenderer` |
| `VisibilityShading` | `WorldRenderer` visibility shading |
| `Shading` | `WorldRenderer` forward pass |
| `ClosestHit`, `AnyHit` | `PathTracer` hit groups |
| `ShadowAnyHit` | Ray traced shadows |
| `ReflectionClosestHit`, `ReflectionAnyHit` | `RayTracedReflectionRenderer` hit groups |

On load, `RenderSystem::RegisterShadingPipelines(material)` calls `RegisterShadingPipeline` on the ray tracing scene and every renderer in every pipeline, so each one can build its material-specific pipeline or hit group.

A material instance (`MaterialInstanceDescriptor`) references a material and sets its property values. Each instance's values live in the material's GPU buffer and are addressed by an instance index, which is how shaders load them (`LoadMaterialInstance`). `MeshRenderer::materials` lists material instances, indexed by each submesh's material slot; a submesh whose slot is missing is not drawn.

## Writing a Renderer

A complete renderer that blurs the world color and publishes the result for later renderers:

```cpp
// BlurRenderer.h
#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct BlurData
{
	TextureHandle blurredColor;
};

class BlurRenderer final : public IRenderer
{
public:

	BlurRenderer(float radius)
		: mRadius(radius)
	{

	}

protected:

	virtual void OnCreate(const RenderContext& context) override
	{
		mPipeline = context.device->CreateComputePipeline({ .entryPoint = "blurComputeShader" });
	}

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::PostProcess; }

private:

	ComputePipelineHandle mPipeline;

	float mRadius;

};

} // namespace Gleam
```

```cpp
// BlurRenderer.cpp
void BlurRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	if (sceneData.world == nullptr)
	{
		return;
	}

	const auto& worldData = blackboard.Get<WorldRenderingData>();

	struct BlurPassData
	{
		TextureHandle source;
		TextureHandle destination;
	};

	const auto& passData = graph.AddComputePass<BlurPassData>("Blur", [&](RenderGraphBuilder& builder, BlurPassData& data)
	{
		RenderTextureDescriptor desc = graph.GetDescriptor(worldData.colorTarget);
		desc.name = "Blurred Color";
		data.destination = builder.WriteTexture(builder.CreateTexture(desc));
		data.source = builder.ReadTexture(worldData.colorTarget);
	},
	[this](const CommandBuffer* cmd, const BlurPassData& data)
	{
		const auto& destination = data.destination.GetTexture();
		const auto size = destination.GetDescriptor().size;

		BlurConstants constants;
		constants.source = data.source;
		constants.destination = destination.GetUnorderedAccessView();
		constants.resolution = uint2(static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height));
		constants.radius = mRadius;

		cmd->BindComputePipeline(mPipeline);
		cmd->SetPushConstant(constants);
		cmd->Dispatch(Math::DivideRoundingUp(constants.resolution.x, 8u), Math::DivideRoundingUp(constants.resolution.y, 8u), 1u);
	});

	blackboard.Add(BlurData{ .blurredColor = passData.destination });
}
```

```cpp
renderSystem->GetRenderPipeline(RenderPath::Default)->AddRenderer<BlurRenderer>(4.0f);
```

Checklist:

1. Put shared structs in `Shaders/ShaderTypes.h` and the entry point in a `.hlsl` under `Shaders/`. The build compiles it automatically.
2. Create pipelines in `OnCreate`, never per frame. The device cache makes repeated creation cheap, but the handle should be stored.
3. Return early from `AddRenderPasses` when `sceneData.world` is `nullptr`, unless the pass draws UI without a scene.
4. Read upstream outputs from the blackboard, and publish your own outputs for downstream renderers.
5. Choose `GetStage()` so that renderers you read from are added before you.
