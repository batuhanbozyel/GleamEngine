#include "gpch.h"
#include "MotionVectorRenderer.h"
#include "DepthPrepass.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/Material.h"

#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void MotionVectorRenderer::OnCreate(const RenderContext& context)
{
    mDevice = context.device;

    for (uint32_t i = 0; i < 3; ++i)
    {
        GraphicsPipelineStateDescriptor pipelineDesc = {
            .blendState = {},
            .depthState = DepthState{ .compareFunction = CompareFunction::Equal, .writeEnabled = false },
            .stencilState = {},
            .cullingMode = static_cast<CullMode>(i),
            .topology = PrimitiveTopology::Triangles,
            .alphaToCoverage = false,
            .wireframe = false,
            .colorFormats = { TextureFormat::R16G16_SFloat },
            .depthFormat = TextureFormat::D16_UNorm,
            .vertexEntry = "motionVectorVertexShader",
            .fragmentEntry = "motionVectorPixelShader"
        };
        mPipelines[i] = mDevice->CreateGraphicsPipeline(pipelineDesc);
    }
}

void MotionVectorRenderer::OnDestroy(const RenderContext& context)
{
}

void MotionVectorRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    const auto& sceneData = blackboard.Get<SceneRenderingData>();
    const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
    const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

    graph.AddRenderPass<MotionVectorData>("MotionVectorRenderer", [&](RenderGraphBuilder& builder, MotionVectorData& passData)
    {
        RenderTextureDescriptor textureDesc;
        textureDesc.name = "MotionVectorRT";
        textureDesc.size = sceneTargetDescriptor.size;
        textureDesc.format = TextureFormat::R16G16_SFloat;
        textureDesc.clearBuffer = true;
        passData.motionVectorTarget = builder.UseColorBuffer(builder.CreateTexture(textureDesc));
        passData.depthTarget = builder.UseDepthBuffer(depthPrepassData.depthTarget, DepthAccess::Read);

        blackboard.Add(passData);
    },
    [this, &sceneData](const CommandBuffer* cmd, const MotionVectorData& passData)
    {
        sceneData.sceneProxy->ForEach([this, cmd, sceneData](const MeshBatch& batch)
        {
            if (batch.numInstances == 0 || batch.material->GetDescriptor().depthState.writeEnabled == false)
            {
                return;
            }

            const auto& pipeline = mPipelines[(uint32_t)batch.material->GetDescriptor().cullingMode];
            const auto globalInstances = sceneData.sceneProxy->GetGlobalInstances();
            const auto globalMeshes = sceneData.sceneProxy->GetGlobalMeshes();

            MotionVectorConstants constants = {};
            constants.instanceBuffer = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();

            cmd->BindGraphicsPipeline(pipeline);
            cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);

            for (uint32_t instanceID = 0; instanceID < batch.numInstances; ++instanceID)
            {
                constants.instanceID = batch.instanceOffset + instanceID;
                const auto& instance = globalInstances[constants.instanceID];

                cmd->SetPushConstant(constants);
                cmd->DrawIndexed(globalMeshes[constants.instanceID].mesh->GetIndexBuffer(), IndexType::UINT32, instance.indexCount, 1, instance.firstIndex);
            }
        });
    });
}
