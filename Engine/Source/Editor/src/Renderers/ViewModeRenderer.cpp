#include "ViewModeRenderer.h"
#include "ShaderTypes.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderSurface.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Renderers/DepthPrepass.h"
#include "Renderer/Renderers/SunShadowRenderer.h"
#include "Renderer/Renderers/GBufferResolveRenderer.h"
#include "Renderer/Renderers/AmbientOcclusionRenderer.h"

#include "World/Systems/RenderSceneProxy.h"

using namespace GEditor;

void ViewModeRenderer::OnCreate(const Gleam::RenderContext& context)
{
    mDevice = context.device;

    Gleam::GraphicsPipelineStateDescriptor pipelineState;
    pipelineState.colorFormats = { context.surface->GetFormat() };
    pipelineState.vertexEntry = "fullscreenTriangleVertexShader";
    pipelineState.fragmentEntry = "viewModeFragmentShader";
    mPipeline = context.device->CreateGraphicsPipeline(pipelineState);
}

void ViewModeRenderer::AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard)
{
    if (mMode == ViewMode::Lit)
    {
        return;
    }

	if (not blackboard.Get<Gleam::SceneRenderingData>().world)
	{
		return;
	}

	Gleam::TextureHandle source;
	switch (mMode)
	{
		case ViewMode::ShadingNormal:
		{
			source = blackboard.Get<Gleam::GBufferData>().shadingNormalTarget;
			break;
		}
		case ViewMode::GeometryNormal:
		{
			source = blackboard.Get<Gleam::GBufferData>().geometryNormalTarget;
			break;
		}
		case ViewMode::Depth:
		{
			source = blackboard.Get<Gleam::DepthPrepassData>().depthTarget;
			break;
		}
		case ViewMode::MotionVectors:
		{
			source = blackboard.Get<Gleam::GBufferData>().motionVectorTarget;
			break;
		}
		case ViewMode::Roughness:
		{
			source = blackboard.Get<Gleam::GBufferData>().roughnessTarget;
			break;
		}
		case ViewMode::ShadowMask:
		{
			if (blackboard.Has<Gleam::SunShadowData>())
			{
				source = blackboard.Get<Gleam::SunShadowData>().shadowMask;
			}
			break;
		}
		case ViewMode::AmbientOcclusion:
		{
			if (blackboard.Has<Gleam::AmbientOcclusionData>())
			{
				source = blackboard.Get<Gleam::AmbientOcclusionData>().aoTarget;
			}
			break;
		}
		case ViewMode::VisibilityIDs:
		case ViewMode::MeshletVisualization:
		case ViewMode::BatchIDs:
		{
			source = blackboard.Get<Gleam::DepthPrepassData>().visibilityBuffer;
			break;
		}
		default:
		{
			return;
		}
	}

	if (not source.IsValid())
	{
		return;
	}

	struct PassData
	{
		Gleam::TextureHandle target;
		Gleam::TextureHandle source;
		ViewMode mode;
	};

	graph.AddRenderPass<PassData>("ViewModeRenderer::Visualize", [&](Gleam::RenderGraphBuilder& builder, PassData& passData)
	{
		const auto& sceneData = blackboard.Get<Gleam::SceneRenderingData>();
		passData.target = builder.UseColorBuffer(sceneData.sceneTarget);
		passData.source = builder.ReadTexture(source);
		passData.mode = mMode;
	},
		[this, &blackboard](const Gleam::CommandBuffer* cmd, const PassData& passData)
	{
		const auto& sceneData = blackboard.Get<Gleam::SceneRenderingData>();

		ViewModeUniforms uniforms;
		uniforms.sourceTexture = passData.source;
		uniforms.mode = static_cast<uint32_t>(passData.mode);

		cmd->BindGraphicsPipeline(mPipeline);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetPushConstant(uniforms);
		cmd->Draw(3);
	});
}
