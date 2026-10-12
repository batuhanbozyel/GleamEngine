#include "gpch.h"
#include "LineRenderer.h"
#include "WorldRenderer.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/WindowSystem.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

using namespace Gleam;

void LineRenderer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
	mAllocator = context.allocator;

	GraphicsPipelineStateDescriptor pipelineState;
	pipelineState.blendState.enabled = true;
	pipelineState.blendState.colorBlendOperation = BlendOp::Add;
	pipelineState.blendState.alphaBlendOperation = BlendOp::Add;
	pipelineState.blendState.sourceColorBlendMode = BlendMode::SrcAlpha;
	pipelineState.blendState.sourceAlphaBlendMode = BlendMode::One;
	pipelineState.blendState.destinationColorBlendMode = BlendMode::OneMinusSrcAlpha;
	pipelineState.blendState.destinationAlphaBlendMode = BlendMode::OneMinusSrcAlpha;
	pipelineState.colorFormats = { TextureFormat::R16G16B16A16_SFloat };
	pipelineState.depthFormat = TextureFormat::D32_SFloat;
	pipelineState.vertexEntry = "lineVertexShader";
	pipelineState.fragmentEntry = "lineFragmentShader";
	mPipeline = mDevice->CreateGraphicsPipeline(pipelineState);

	pipelineState.depthState.compareFunction = CompareFunction::Less;
	mDepthPipeline = mDevice->CreateGraphicsPipeline(pipelineState);
}

void LineRenderer::OnDestroy(const RenderContext& context)
{
	if (mLineBuffer.IsValid())
	{
		context.device->Dispose(mAllocator, mLineBuffer, BarrierStage::None);
	}
}

void LineRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	if (not blackboard.Get<SceneRenderingData>().world)
	{
		return;
	}

	size_t lineCount = mLines.size() + mDepthLines.size();
	if (lineCount == 0)
	{
		return;
	}

	size_t bufferSize = lineCount * sizeof(LineData);
	if (mLineBuffer.GetDescriptor().size < bufferSize)
	{
		if (mLineBuffer.IsValid())
		{
			mDevice->Dispose(mAllocator, mLineBuffer, BarrierStage::None);
		}

		BufferDescriptor descriptor{ .name = "LineBuffer", .memoryType = MemoryType::CPU, .size = Math::RoundUpTo(bufferSize, (size_t)65536ull) };
		mLineBuffer = mDevice->CreateBuffer(mAllocator, descriptor);
	}

	void* lineBufferPtr = mLineBuffer.GetContents();
	size_t depthLineBufferOffset = mLines.size() * sizeof(LineData);

	memcpy(lineBufferPtr, mLines.data(), mLines.size() * sizeof(LineData));
	memcpy(OffsetPointer(lineBufferPtr, depthLineBufferOffset), mDepthLines.data(), mDepthLines.size() * sizeof(LineData));

	struct DrawPassData
	{
		TextureHandle colorTarget;
		TextureHandle depthTarget;
	};

	graph.AddRenderPass<DrawPassData>("LineRenderer::DrawPass", [&](RenderGraphBuilder& builder, DrawPassData& passData)
	{
		const auto& worldData = blackboard.Get<WorldRenderingData>();
		passData.colorTarget = builder.UseColorBuffer(worldData.colorTarget);
		passData.depthTarget = builder.UseDepthBuffer(worldData.depthTarget, DepthAccess::Read);
	},
	[this, &blackboard, depthLineBufferOffset](const CommandBuffer* cmd, const DrawPassData& passData)
	{
		const auto& sceneData = blackboard.Get<SceneRenderingData>();

		LineShaderResources resources;
		resources.lineBuffer = mLineBuffer.GetResourceView();
		resources.thicknessScale = Globals::Engine->GetSubsystem<WindowSystem>()->GetDisplayScale();

		auto drawLines = [&](size_t count, size_t offset, bool depthTest)
		{
			resources.lineOffset = static_cast<uint32_t>(offset);
			cmd->BindGraphicsPipeline(depthTest ? mDepthPipeline : mPipeline);
			cmd->SetConstantBuffer(resources, 0);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, 1);
			cmd->Draw(static_cast<uint32_t>(count) * 6);
		};

		if (not mDepthLines.empty())
		{
			drawLines(mDepthLines.size(), depthLineBufferOffset, true);
		}

		if (not mLines.empty())
		{
			drawLines(mLines.size(), 0, false);
		}

		mLines.clear();
		mDepthLines.clear();
	});
}

void LineRenderer::DrawLine(const Float3& start, const Float3& end, Color32 color, float thickness, bool depthTest)
{
	LineData line;
	line.start = start;
	line.thickness = thickness;
	line.end = end;
	line.color = color;

	if (depthTest)
	{
		mDepthLines.push_back(line);
	}
	else
	{
		mLines.push_back(line);
	}
}

void LineRenderer::DrawTriangle(const Float3& v1, const Float3& v2, const Float3& v3, Color32 color, float thickness, bool depthTest)
{
	DrawLine(v1, v2, color, thickness, depthTest);
	DrawLine(v2, v3, color, thickness, depthTest);
	DrawLine(v3, v1, color, thickness, depthTest);
}

void LineRenderer::DrawQuad(const Float3& center, float width, float height, Color32 color, float thickness, bool depthTest)
{
	float halfWidth = width / 2.0f;
	float halfHeight = height / 2.0f;

	Float3 v0{ center.x - halfWidth, center.y, center.z - halfHeight };
	Float3 v1{ center.x + halfWidth, center.y, center.z - halfHeight };
	Float3 v2{ center.x + halfWidth, center.y, center.z + halfHeight };
	Float3 v3{ center.x - halfWidth, center.y, center.z + halfHeight };

	DrawLine(v0, v1, color, thickness, depthTest);
	DrawLine(v1, v2, color, thickness, depthTest);
	DrawLine(v2, v3, color, thickness, depthTest);
	DrawLine(v3, v0, color, thickness, depthTest);
}

void LineRenderer::DrawBoundingBox(const BoundingBox& boundingBox, Color32 color, float thickness, bool depthTest)
{
	const Float3& min = boundingBox.min;
	const Float3& max = boundingBox.max;

	Float3 v1(max.x, min.y, min.z);
	Float3 v2(max.x, max.y, min.z);
	Float3 v3(min.x, max.y, min.z);
	Float3 v4(min.x, min.y, max.z);
	Float3 v5(max.x, min.y, max.z);
	Float3 v6(min.x, max.y, max.z);

	DrawLine(min, v1, color, thickness, depthTest);
	DrawLine(v1, v2, color, thickness, depthTest);
	DrawLine(v2, v3, color, thickness, depthTest);
	DrawLine(v3, min, color, thickness, depthTest);
	DrawLine(v4, v5, color, thickness, depthTest);
	DrawLine(v5, max, color, thickness, depthTest);
	DrawLine(max, v6, color, thickness, depthTest);
	DrawLine(v6, v4, color, thickness, depthTest);
	DrawLine(min, v4, color, thickness, depthTest);
	DrawLine(v1, v5, color, thickness, depthTest);
	DrawLine(v2, max, color, thickness, depthTest);
	DrawLine(v3, v6, color, thickness, depthTest);
}

void LineRenderer::DrawBoundingBox(const BoundingBox& boundingBox, const Float4x4& transform, Color32 color, float thickness, bool depthTest)
{
	const Float3& min = boundingBox.min;
	const Float3& max = boundingBox.max;

	Float3 v0(transform * min);
	Float3 v1(transform * Float3(max.x, min.y, min.z));
	Float3 v2(transform * Float3(max.x, max.y, min.z));
	Float3 v3(transform * Float3(min.x, max.y, min.z));
	Float3 v4(transform * Float3(min.x, min.y, max.z));
	Float3 v5(transform * Float3(max.x, min.y, max.z));
	Float3 v6(transform * Float3(min.x, max.y, max.z));
	Float3 v7(transform * max);

	DrawLine(v0, v1, color, thickness, depthTest);
	DrawLine(v1, v2, color, thickness, depthTest);
	DrawLine(v2, v3, color, thickness, depthTest);
	DrawLine(v3, v0, color, thickness, depthTest);
	DrawLine(v4, v5, color, thickness, depthTest);
	DrawLine(v5, v7, color, thickness, depthTest);
	DrawLine(v7, v6, color, thickness, depthTest);
	DrawLine(v6, v4, color, thickness, depthTest);
	DrawLine(v0, v4, color, thickness, depthTest);
	DrawLine(v1, v5, color, thickness, depthTest);
	DrawLine(v2, v7, color, thickness, depthTest);
	DrawLine(v3, v6, color, thickness, depthTest);
}
