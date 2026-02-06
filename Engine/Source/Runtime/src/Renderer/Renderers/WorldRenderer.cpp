//
//  WorldRenderer.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#include "gpch.h"
#include "WorldRenderer.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Renderers/ReflectionProbeRenderer.h"

#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void WorldRenderer::OnCreate(RenderContext& context)
{
	mDevice = context.device;

	// BRDF Lut
	{
		ComputePipelineStateDescriptor pipelineState;
		pipelineState.entryPoint = "integrateBRDFShader";
		mBRDFLutPipeline = context.device->CreateComputePipeline(pipelineState);

		TextureDescriptor textureDesc;
		textureDesc.name = "BRDF LUT";
		textureDesc.dimension = TextureDimension::Texture2D;
		textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
		textureDesc.usage = TextureUsage_Storage | TextureUsage_Sampled;
		textureDesc.size = { BRDF_LUT_SIZE, BRDF_LUT_SIZE };
		mBRDFLutTexture = context.device->CreateTexture(context.allocator, textureDesc);
	}
}

void WorldRenderer::OnDestroy(RenderContext& context)
{
	context.device->Dispose(context.allocator, mBRDFLutTexture);
}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& reflectionProbeData = blackboard.Get<ReflectionProbePassData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);
	auto brdfLut = graph.ImportTexture(mBRDFLutTexture);

	static bool mBakeBRDFLut = true;
	if (mBakeBRDFLut)
	{
		struct BRDFLutData
		{
			TextureHandle brdfLut;
		};
		graph.AddComputePass<BRDFLutData>("WorldRenderer::BRDFLut", [&](RenderGraphBuilder& builder, BRDFLutData& passData)
		{
			passData.brdfLut = builder.WriteTexture(brdfLut);
			brdfLut = passData.brdfLut;
		},
		[this, blackboard](const CommandBuffer* cmd, const BRDFLutData& passData)
		{
			cmd->BindComputePipeline(mBRDFLutPipeline);
			cmd->SetPushConstant(BRDFLutConstants{ .targetTexture = mBRDFLutTexture.GetResourceView() });
			cmd->Dispatch(Math::DivideRoundingUp(BRDF_LUT_SIZE, 16), Math::DivideRoundingUp(BRDF_LUT_SIZE, 16), 1);
		});
		mBakeBRDFLut = false;
	}
	
    graph.AddRenderPass<WorldRenderingData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
    {
        RenderTextureDescriptor textureDesc;
        textureDesc.name = "SceneColorRT";
        textureDesc.size = sceneTargetDescriptor.size;
        textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
        textureDesc.clearBuffer = true;
        passData.colorTarget = builder.CreateTexture(textureDesc);
        
        textureDesc.name = "SceneDepthRT";
        textureDesc.format = TextureFormat::D16_UNorm;
        passData.depthTarget = builder.CreateTexture(textureDesc);
        
        passData.colorTarget = builder.UseColorBuffer(passData.colorTarget);
        passData.depthTarget = builder.UseDepthBuffer(passData.depthTarget, DepthAccess::Write);

		passData.transmittanceLut = builder.ReadTexture(sceneData.atmosphere.transmittanceLut);
		passData.multiScatterLut = builder.ReadTexture(sceneData.atmosphere.multiScatterLut);
		passData.brdfLut = builder.ReadTexture(brdfLut);

		passData.specularReflection = builder.ReadTexture(reflectionProbeData.specularReflection);
		passData.diffuseReflection = builder.ReadTexture(reflectionProbeData.diffuseReflection);
        blackboard.Add(passData);
    },
    [this, blackboard](const CommandBuffer* cmd, const WorldRenderingData& passData)
    {
        const auto& sceneData = blackboard.Get<SceneRenderingData>();
        sceneData.sceneProxy->ForEach([this, cmd, passData, sceneData](const MeshBatch& batch)
        {
            const auto& materialBuffer = batch.material->GetBuffer();
            const auto& pipeline = mShadingPipelines[batch.material->GetPipelineHash()];

			MeshPassResources resources = {};
			resources.instanceBuffer = batch.instanceBuffer.GetResourceView();
			resources.materialBuffer = materialBuffer.GetResourceView();
			resources.brdfTexture = passData.brdfLut;
			resources.diffuseReflectionTexture = passData.diffuseReflection;
			resources.specularReflectionTexture = passData.specularReflection;

			cmd->BindGraphicsPipeline(pipeline);
			cmd->SetConstantBuffer(resources, MESH_PASS_RESOURCES_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);

			for (uint32_t instanceID = 0; instanceID < batch.numInstances; ++instanceID)
			{
				const auto& instance = batch.instances[instanceID];
				cmd->SetConstantBuffer(instance, MESH_INSTANCE_DATA_BINDING_SLOT);
				cmd->DrawIndexed(batch.meshes[instanceID]->GetIndexBuffer(), IndexType::UINT32, instance.indexCount, 1, instance.firstIndex);
			}
        });
    });
}

void WorldRenderer::RegisterShadingPipeline(const MaterialDescriptor& material, uint32_t hash)
{
	auto it = mShadingPipelines.find(hash);
	if (it == mShadingPipelines.end())
	{
		GraphicsPipelineStateDescriptor pipelineDesc = {
			.blendState = material.blendState,
			.depthState = material.depthState,
			.stencilState = material.stencilState,
			.cullingMode = material.cullingMode,
			.topology = PrimitiveTopology::Triangles,
			.alphaToCoverage = false,
			.wireframe = false,
			.colorFormats = { TextureFormat::R16G16B16A16_SFloat },
			.depthFormat = TextureFormat::D16_UNorm,
			.vertexEntry = "meshVertexShader",
			.fragmentEntry = material.surfaceShader
		};
		auto pipeline = mDevice->CreateGraphicsPipeline(pipelineDesc);

		mShadingPipelines.emplace_hint(it, eastl::piecewise_construct,
										   eastl::forward_as_tuple(hash),
										   eastl::forward_as_tuple(pipeline));
	}
}
