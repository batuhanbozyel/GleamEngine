#include "gpch.h"
#include "BRDFRenderer.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

using namespace Gleam;

void BRDFRenderer::OnCreate(const RenderContext& context)
{
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

	// GGX Ess Lut
	{
		ComputePipelineStateDescriptor pipelineState;
		pipelineState.entryPoint = "integrateEssShader";
		mGGXEssLutPipeline = context.device->CreateComputePipeline(pipelineState);

		TextureDescriptor textureDesc;
		textureDesc.name = "Multiscatter GGX Ess LUT";
		textureDesc.dimension = TextureDimension::Texture2D;
		textureDesc.format = TextureFormat::R16_SFloat;
		textureDesc.usage = TextureUsage_Storage | TextureUsage_Sampled;
		textureDesc.size = { BRDF_LUT_SIZE, BRDF_LUT_SIZE };
		mGGXEssLutTexture = context.device->CreateTexture(context.allocator, textureDesc);
	}

	// GGX EAvg Lut
	{
		ComputePipelineStateDescriptor pipelineState;
		pipelineState.entryPoint = "integrateEAvgShader";
		mGGXEAvgLutPipeline = context.device->CreateComputePipeline(pipelineState);

		TextureDescriptor textureDesc;
		textureDesc.name = "Multiscatter GGX EAvg LUT";
		textureDesc.dimension = TextureDimension::Texture2D;
		textureDesc.format = TextureFormat::R16_SFloat;
		textureDesc.usage = TextureUsage_Storage | TextureUsage_Sampled;
		textureDesc.size = { BRDF_LUT_SIZE, 1 };
		mGGXEAvgLutTexture = context.device->CreateTexture(context.allocator, textureDesc);
	}
}

void BRDFRenderer::OnDestroy(const RenderContext& context)
{
	context.device->Dispose(context.allocator, mBRDFLutTexture, BarrierStage::None);
	context.device->Dispose(context.allocator, mGGXEssLutTexture, BarrierStage::None);
	context.device->Dispose(context.allocator, mGGXEAvgLutTexture, BarrierStage::None);
}

void BRDFRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	auto brdfLut = graph.ImportTexture(mBRDFLutTexture);
	auto ggxEssLut = graph.ImportTexture(mGGXEssLutTexture);
	auto ggxEAvgLut = graph.ImportTexture(mGGXEAvgLutTexture);

	static bool mBakeLuts = true;
	if (mBakeLuts)
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
		[this](const CommandBuffer* cmd, const BRDFLutData& passData)
		{
			cmd->BindComputePipeline(mBRDFLutPipeline);
			cmd->SetPushConstant(BRDFLutConstants{ .targetTexture = mBRDFLutTexture.GetResourceView() });
			cmd->Dispatch(Math::DivideRoundingUp(BRDF_LUT_SIZE, 16), Math::DivideRoundingUp(BRDF_LUT_SIZE, 16), 1);
		});

		struct GGXEssData
        {
            TextureHandle essLut;
        };
        graph.AddComputePass<GGXEssData>("WorldRenderer::Multiscatter GGX Ess LUT",
        [&](RenderGraphBuilder& builder, GGXEssData& passData)
        {
            passData.essLut = builder.WriteTexture(ggxEssLut);
            ggxEssLut = passData.essLut;
        },
        [this](const CommandBuffer* cmd, const GGXEssData&)
        {
            cmd->BindComputePipeline(mGGXEssLutPipeline);
            cmd->SetPushConstant(MultiscatterGGXLutConstants{ .targetTexture = mGGXEssLutTexture.GetResourceView(), .essTexture = mGGXEssLutTexture.GetResourceView() });
            cmd->Dispatch(Math::DivideRoundingUp(BRDF_LUT_SIZE, 16), Math::DivideRoundingUp(BRDF_LUT_SIZE, 16), 1);
        });

		struct GGXEAvgData
		{
			TextureHandle eAvgLut;
			TextureHandle essLut;
		};
		graph.AddComputePass<GGXEAvgData>("WorldRenderer::Multiscatter GGX EAvg LUT",
		[&](RenderGraphBuilder& builder, GGXEAvgData& passData)
		{
			passData.eAvgLut = builder.WriteTexture(ggxEAvgLut);
			passData.essLut = builder.ReadTexture(ggxEssLut);
			ggxEAvgLut = passData.eAvgLut;
		},
		[this](const CommandBuffer* cmd, const GGXEAvgData&)
		{
			cmd->BindComputePipeline(mGGXEAvgLutPipeline);
			cmd->SetPushConstant(MultiscatterGGXLutConstants{ .targetTexture = mGGXEAvgLutTexture.GetResourceView(), .essTexture = mGGXEssLutTexture.GetResourceView() });
			cmd->Dispatch(Math::DivideRoundingUp(BRDF_LUT_SIZE, 16), 1, 1);
		});
		mBakeLuts = false;
	}
	blackboard.Add(BRDFData{ .brdfLut = brdfLut, .ggxEssLut = ggxEssLut, .ggxEAvgLut = ggxEAvgLut });
}
