#include "ViewModeRenderer.h"
#include "ShaderTypes.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderSurface.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Renderers/DepthPrepass.h"
#include "Renderer/Renderers/SunShadowRenderer.h"

using namespace GEditor;

void ViewModeRenderer::OnCreate(const Gleam::RenderContext& context)
{
    Gleam::GraphicsPipelineStateDescriptor pipelineState;
    pipelineState.colorFormats = { context.surface->GetFormat() };
    pipelineState.vertexEntry = "fullscreenTriangleVertexShader";
    pipelineState.fragmentEntry = "viewModeFragmentShader";
    mPipeline = context.device->CreateGraphicsPipeline(pipelineState);
}

void ViewModeRenderer::AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard)
{
    if (mMode == Gleam::ViewMode::Lit)
    {
        return;
    }
	
    Gleam::TextureHandle source;
    switch (mMode)
    {
        case Gleam::ViewMode::WorldNormal:
        {
			source = blackboard.Get<Gleam::DepthPrepassData>().normalTarget;
            break;
        }
        case Gleam::ViewMode::Depth:
        {
			source = blackboard.Get<Gleam::DepthPrepassData>().depthTarget;
            break;
        }
        case Gleam::ViewMode::MotionVectors:
        {
			source = blackboard.Get<Gleam::DepthPrepassData>().motionVectorTarget;
            break;
        }
        case Gleam::ViewMode::ShadowMask:
        {
			source = blackboard.Get<Gleam::SunShadowData>().shadowMask;
            break;
        }
        default:
        {
            return;
        }
    }

    struct PassData
    {
        Gleam::TextureHandle target;
        Gleam::TextureHandle source;
        Gleam::ViewMode mode;
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
        cmd->SetConstantBuffer(sceneData.camera.uniforms, 0);
        cmd->SetPushConstant(uniforms);
        cmd->Draw(3);
    });
}
