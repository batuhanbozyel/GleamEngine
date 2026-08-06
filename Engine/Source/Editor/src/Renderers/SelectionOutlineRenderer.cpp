#include "SelectionOutlineRenderer.h"
#include "ShaderTypes.h"
#include "Selection/SelectionSystem.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderSurface.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/CopyCommandBuffer.h"
#include "Renderer/Renderers/DepthPrepass.h"

using namespace GEditor;

void SelectionOutlineRenderer::OnCreate(const Gleam::RenderContext& context)
{
	mCopyCommandBuffer = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>()->GetCopyCommandBuffer();

	Gleam::ComputePipelineStateDescriptor maskDesc;
	maskDesc.entryPoint = "selectionMaskShader";
	mMaskPipeline = context.device->CreateComputePipeline(maskDesc);

	Gleam::GraphicsPipelineStateDescriptor outlineDesc;
	outlineDesc.colorFormats = { context.surface->GetFormat() };
	outlineDesc.vertexEntry = "fullscreenTriangleVertexShader";
	outlineDesc.fragmentEntry = "selectionOutlineFragmentShader";
	outlineDesc.blendState.enabled = true;
	outlineDesc.blendState.colorBlendOperation = Gleam::BlendOp::Add;
	outlineDesc.blendState.alphaBlendOperation = Gleam::BlendOp::Add;
	outlineDesc.blendState.sourceColorBlendMode = Gleam::BlendMode::SrcAlpha;
	outlineDesc.blendState.sourceAlphaBlendMode = Gleam::BlendMode::One;
	outlineDesc.blendState.destinationColorBlendMode = Gleam::BlendMode::OneMinusSrcAlpha;
	outlineDesc.blendState.destinationAlphaBlendMode = Gleam::BlendMode::OneMinusSrcAlpha;
	mOutlinePipeline = context.device->CreateGraphicsPipeline(outlineDesc);

	Gleam::BufferDescriptor bufferDesc;
	bufferDesc.name = "SelectionInstanceMask";
	bufferDesc.size = InstanceMaskBytes;
	mInstanceMaskBuffer = context.device->CreateBuffer(context.allocator, bufferDesc);
}

void SelectionOutlineRenderer::OnDestroy(const Gleam::RenderContext& context)
{
	context.device->Dispose(context.allocator, mInstanceMaskBuffer, Gleam::BarrierStage::None);
}

void SelectionOutlineRenderer::AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard)
{
	const auto& instanceMask = mSelection->GetInstanceMask();
	if (instanceMask.empty())
	{
		return;
	}

	mCopyCommandBuffer->Commit(mInstanceMaskBuffer, instanceMask.data(), instanceMask.size() * sizeof(uint32_t), 0);

	const auto& sceneData = blackboard.Get<Gleam::SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<Gleam::DepthPrepassData>();
	const auto& targetSize = graph.GetDescriptor(sceneData.sceneTarget).size;

	const auto targetWidth = static_cast<uint32_t>(targetSize.width);
	const auto targetHeight = static_cast<uint32_t>(targetSize.height);

	struct MaskPassData
	{
		Gleam::TextureHandle visibilityBuffer;
		Gleam::TextureHandle selectionMask;
	};

	const auto& maskData = graph.AddComputePass<MaskPassData>("SelectionOutline::Mask", [&](Gleam::RenderGraphBuilder& builder, MaskPassData& passData)
	{
		Gleam::RenderTextureDescriptor maskDesc;
		maskDesc.name = "Selection Mask";
		maskDesc.size = targetSize;
		maskDesc.format = Gleam::TextureFormat::R8_UNorm;
		passData.selectionMask = builder.WriteTexture(builder.CreateTexture(maskDesc));

		passData.visibilityBuffer = builder.ReadTexture(depthPrepassData.visibilityBuffer);
	},
	[this, targetWidth, targetHeight](const Gleam::CommandBuffer* cmd, const MaskPassData& passData)
	{
		SelectionMaskConstants constants = {};
		constants.visibilityBuffer = passData.visibilityBuffer;
		constants.instanceMaskBuffer = mInstanceMaskBuffer.GetResourceView();
		constants.selectionMask = passData.selectionMask;
		constants.targetWidth = targetWidth;
		constants.targetHeight = targetHeight;

		cmd->BindComputePipeline(mMaskPipeline);
		cmd->SetPushConstant(constants);
		cmd->Dispatch(Gleam::Math::DivideRoundingUp(targetWidth, SELECTION_MASK_GROUP_SIZE_X),
					  Gleam::Math::DivideRoundingUp(targetHeight, SELECTION_MASK_GROUP_SIZE_Y), 1u);
	});

	struct OutlinePassData
	{
		Gleam::TextureHandle renderTarget;
		Gleam::TextureHandle selectionMask;
	};

	graph.AddRenderPass<OutlinePassData>("SelectionOutline::Composite", [&](Gleam::RenderGraphBuilder& builder, OutlinePassData& passData)
	{
		passData.renderTarget = builder.UseColorBuffer(sceneData.sceneTarget);
		passData.selectionMask = builder.ReadTexture(maskData.selectionMask);
	},
	[this](const Gleam::CommandBuffer* cmd, const OutlinePassData& passData)
	{
		SelectionOutlineUniforms uniforms = {};
		uniforms.selectionMask = passData.selectionMask;
		uniforms.outlineWidth = mOutlineWidth;
		uniforms.outlineColor = mOutlineColor;

		cmd->BindGraphicsPipeline(mOutlinePipeline);
		cmd->SetPushConstant(uniforms);
		cmd->Draw(3);
	});
}
