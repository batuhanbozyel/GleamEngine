#include "ViewModeRenderer.h"
#include "ShaderTypes.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderSurface.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Renderers/DepthPrepass.h"
#include "Renderer/Renderers/SunShadowRenderer.h"

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

    if (context.device->GetFeatures().meshShaders)
    {
		for (uint32_t cullingMode = 0; cullingMode < 3; ++cullingMode)
		{
			Gleam::MeshPipelineStateDescriptor meshPipelineState = {
				.blendState = {},
				.depthState = Gleam::DepthState{.compareFunction = Gleam::CompareFunction::LessEqual, .writeEnabled = false },
				.stencilState = Gleam::StencilState{.enabled = false },
				.cullingMode = (Gleam::CullMode)cullingMode,
				.alphaToCoverage = false,
				.wireframe = false,
				.colorFormats = { context.surface->GetFormat() },
				.depthFormat = Gleam::TextureFormat::D32_SFloat,
				.meshEntry = "meshletVisMeshShader",
				.amplificationEntry = "meshletVisAmplificationShader",
				.fragmentEntry = "meshletVisFragmentShader"
			};
			mMeshletVisPipelines[cullingMode] = context.device->CreateMeshPipeline(meshPipelineState);
		}
    }
}

void ViewModeRenderer::AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard)
{
    if (mMode == Gleam::ViewMode::Lit)
    {
        return;
    }

    if (mMode == Gleam::ViewMode::MeshletVisualization)
    {
        AddMeshletVisualizationPass(graph, blackboard);
    }
    else
    {
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
}

void ViewModeRenderer::AddMeshletVisualizationPass(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard)
{
    if (not mDevice->GetFeatures().meshShaders)
    {
        return;
    }

    struct PassData
    {
        Gleam::TextureHandle target;
        Gleam::TextureHandle depth;
    };

    graph.AddRenderPass<PassData>("ViewModeRenderer::MeshletVisualization", [&](Gleam::RenderGraphBuilder& builder, PassData& passData)
    {
        const auto& sceneData = blackboard.Get<Gleam::SceneRenderingData>();
        const auto& depthData = blackboard.Get<Gleam::DepthPrepassData>();
        passData.target = builder.UseColorBuffer(sceneData.sceneTarget);
        passData.depth = builder.UseDepthBuffer(depthData.depthTarget, Gleam::DepthAccess::Read);
    },
    [this, &blackboard](const Gleam::CommandBuffer* cmd, const PassData& passData)
    {
        const auto& sceneData = blackboard.Get<Gleam::SceneRenderingData>();
        sceneData.sceneProxy->ForEach([cmd, &sceneData, this](const Gleam::MeshBatch& batch)
        {
            if (batch.numInstances == 0)
            {
                return;
            }

			cmd->BindMeshPipeline(mMeshletVisPipelines[(uint32_t)batch.material->GetDescriptor().cullingMode]);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);

            const auto globalInstances = sceneData.sceneProxy->GetGlobalInstances();

            MeshletVisualizationConstants constants = {};
            constants.instanceBuffer = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();

            for (uint32_t instanceID = 0; instanceID < batch.numInstances; ++instanceID)
            {
                constants.instanceID = batch.instanceOffset + instanceID;
                const auto& instance = globalInstances[constants.instanceID];
                cmd->SetPushConstant(constants);
                cmd->DispatchMesh(Gleam::Math::DivideRoundingUp(instance.meshletCount, MESH_AMPLIFICATION_THREADS), 1, 1);
            }
        });
    });
}
