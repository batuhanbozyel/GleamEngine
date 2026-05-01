//
//  WorldRenderer.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#include "gpch.h"
#include "WorldRenderer.h"
#include "BRDFRenderer.h"
#include "DepthPrepass.h"
#include "SunShadowRenderer.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Renderers/ReflectionProbeRenderer.h"

#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void WorldRenderer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
}

void WorldRenderer::OnDestroy(const RenderContext& context)
{

}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);
	
    graph.AddRenderPass<WorldRenderingData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
    {
        RenderTextureDescriptor textureDesc;
        textureDesc.name = "SceneColorRT";
        textureDesc.size = sceneTargetDescriptor.size;
        textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
        textureDesc.clearBuffer = true;
        passData.colorTarget = builder.UseColorBuffer(builder.CreateTexture(textureDesc));
        passData.depthTarget = builder.UseDepthBuffer(depthPrepassData.depthTarget, DepthAccess::Read);

		const auto& brdfData = blackboard.Get<BRDFData>();
		passData.brdfLut = builder.ReadTexture(brdfData.brdfLut);
		passData.ggxEssLut = builder.ReadTexture(brdfData.ggxEssLut);
		passData.ggxEAvgLut = builder.ReadTexture(brdfData.ggxEAvgLut);
		
		if (sceneData.atmosphere.transmittanceLut.IsValid() && sceneData.atmosphere.multiScatterLut.IsValid())
		{
			passData.transmittanceLut = builder.ReadTexture(sceneData.atmosphere.transmittanceLut);
			passData.multiScatterLut = builder.ReadTexture(sceneData.atmosphere.multiScatterLut);
		}
		
		const auto& reflectionProbeData = blackboard.Get<ReflectionProbePassData>();
		passData.specularReflection = builder.ReadTexture(reflectionProbeData.specularReflection);
		passData.diffuseReflection = builder.ReadTexture(reflectionProbeData.diffuseReflection);

		if (blackboard.Has<SunShadowData>())
		{
			const auto& sunShadowData = blackboard.Get<SunShadowData>();
			passData.shadowTexture = builder.ReadTexture(sunShadowData.shadowMask);
		}

        blackboard.Add(passData);
    },
    [this, &sceneData](const CommandBuffer* cmd, const WorldRenderingData& passData)
    {
        sceneData.sceneProxy->ForEach([this, cmd, passData, sceneData](const MeshBatch& batch)
        {
			if (batch.numInstances == 0)
			{
				return;
			}

            const auto& pipeline = mShadingPipelines[batch.material->GetPipelineHash()];
			const auto globalInstances = sceneData.sceneProxy->GetGlobalInstances();
			const auto globalMeshes = sceneData.sceneProxy->GetGlobalMeshes();

			MeshShadingConstants constants = {};
			constants.instanceBuffer = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
			constants.brdfTexture = passData.brdfLut;
			constants.ggxEssTexture = passData.ggxEssLut;
			constants.ggxEAvgTexture = passData.ggxEAvgLut;
			constants.diffuseReflectionTexture = passData.diffuseReflection;
			constants.specularReflectionTexture = passData.specularReflection;
			constants.shadowTexture = passData.shadowTexture;

			cmd->BindGraphicsPipeline(pipeline);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);

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

void WorldRenderer::RegisterShadingPipeline(const Material* material)
{
	const auto& materialDesc = material->GetDescriptor();
	auto pipelineHash = material->GetPipelineHash();
	auto it = mShadingPipelines.find(pipelineHash);
	if (it == mShadingPipelines.end())
	{
		DepthState depthState;
		StencilState stencilState;
		if (materialDesc.depthState.writeEnabled)
		{
			// If depth writing is enabled,
			// we assume this material is opaque and depth writing is done via depth prepass.
			// Otherwise, we need to use the depth/stencil state specified in the material descriptor for correct rendering of transparent materials.
			depthState = DepthState{ .compareFunction = CompareFunction::Equal, .writeEnabled = false };
			stencilState = StencilState{ .enabled = false };
		}
		else
		{
			depthState = materialDesc.depthState;
			stencilState = materialDesc.stencilState;
		}

		GraphicsPipelineStateDescriptor pipelineDesc = {
			.blendState = materialDesc.blendState,
			.depthState = depthState,
			.stencilState = stencilState,
			.cullingMode = materialDesc.cullingMode,
			.topology = PrimitiveTopology::Triangles,
			.alphaToCoverage = false,
			.wireframe = false,
			.colorFormats = { TextureFormat::R16G16B16A16_SFloat },
			.depthFormat = TextureFormat::D16_UNorm,
			.vertexEntry = "meshVertexShader",
			.fragmentEntry = materialDesc.surfaceShader + "Shading"
		};
		auto pipeline = mDevice->CreateGraphicsPipeline(pipelineDesc);

		mShadingPipelines.emplace_hint(it, eastl::piecewise_construct,
										   eastl::forward_as_tuple(pipelineHash),
										   eastl::forward_as_tuple(pipeline));
	}
}
