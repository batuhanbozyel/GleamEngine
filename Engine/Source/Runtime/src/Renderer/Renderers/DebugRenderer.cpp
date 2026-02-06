#include "gpch.h"
#include "DebugRenderer.h"
#include "WorldRenderer.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

using namespace Gleam;

void DebugRenderer::OnCreate(RenderContext& context)
{
	mDevice = context.device;
	mAllocator = context.allocator;
	// Primitive pipelines
	{
		GraphicsPipelineStateDescriptor pipelineState;
		pipelineState.topology = PrimitiveTopology::Lines;
		pipelineState.depthState.compareFunction = CompareFunction::Always;
		pipelineState.colorFormats = { TextureFormat::R16G16B16A16_SFloat };
		pipelineState.vertexEntry = "debugVertexShader";
		pipelineState.fragmentEntry = "debugFragmentShader";
		mPrimitivePipeline = mDevice->CreateGraphicsPipeline(pipelineState);

		pipelineState.depthState.writeEnabled = true;
		pipelineState.depthFormat = TextureFormat::D16_UNorm;
		pipelineState.depthState.compareFunction = CompareFunction::Less;
		mPrimitiveDepthPipeline = mDevice->CreateGraphicsPipeline(pipelineState);
	}

	// Mesh pipelines
	{
		GraphicsPipelineStateDescriptor pipelineState;
		pipelineState.topology = PrimitiveTopology::Triangles;
		pipelineState.depthState.compareFunction = CompareFunction::Always;
		pipelineState.colorFormats = { TextureFormat::R16G16B16A16_SFloat };
		pipelineState.vertexEntry = "debugMeshVertexShader";
		pipelineState.fragmentEntry = "debugFragmentShader";
		mMeshPipeline = mDevice->CreateGraphicsPipeline(pipelineState);

		pipelineState.depthState.writeEnabled = true;
		pipelineState.depthFormat = TextureFormat::D16_UNorm;
		pipelineState.depthState.compareFunction = CompareFunction::Less;
		mMeshDepthPipeline = mDevice->CreateGraphicsPipeline(pipelineState);
	}
}

void DebugRenderer::OnDestroy(RenderContext& context)
{
	context.device->Dispose(mAllocator, mVertexBuffer);
}

void DebugRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	size_t vertexCount = (mLines.size() + mDepthLines.size()) * 2;
	size_t bufferSize = vertexCount * sizeof(DebugVertex);

	// nothing to render
	if (vertexCount == 0 && mDebugMeshes.empty())
		return;

	if (mVertexBuffer.GetDescriptor().size < bufferSize)
	{
		if (mVertexBuffer.IsValid())
		{
			mDevice->Dispose(mAllocator, mVertexBuffer);
		}

		BufferDescriptor descriptor{ .name = "DebugVertexBuffer", .memoryType = MemoryType::CPU, .size = Math::RoundUpTo(bufferSize, (size_t)65536ull) };
		mVertexBuffer = mDevice->CreateBuffer(mAllocator, descriptor);
	}

	void* vertexBufferPtr = mVertexBuffer.GetContents();
	size_t depthLineBufferOffset = mLines.size() * sizeof(DebugLine);

	memcpy(vertexBufferPtr, mLines.data(), mLines.size() * sizeof(DebugLine));
	memcpy(OffsetPointer(vertexBufferPtr, depthLineBufferOffset), mDepthLines.data(), mDepthLines.size() * sizeof(DebugLine));

	struct DrawPassData
	{
		TextureHandle colorTarget;
		TextureHandle depthTarget;
	};

	graph.AddRenderPass<DrawPassData>("DebugRenderer::DrawPass", [&](RenderGraphBuilder& builder, DrawPassData& passData)
	{
        const auto& worldData = blackboard.Get<WorldRenderingData>();
        passData.colorTarget = builder.UseColorBuffer(worldData.colorTarget);
        passData.depthTarget = builder.UseDepthBuffer(worldData.depthTarget, DepthAccess::Write);
	},
	[this, blackboard](const CommandBuffer* cmd, const DrawPassData& passData)
	{
        DebugShaderResources resources;
        resources.vertexBuffer = mVertexBuffer.GetResourceView();
		const auto& sceneData = blackboard.Get<SceneRenderingData>();
        
		if (!mDepthLines.empty())
		{
			cmd->BindGraphicsPipeline(mPrimitiveDepthPipeline);
			cmd->SetConstantBuffer(resources, 0);
			cmd->SetConstantBuffer(sceneData.camera, 1);
			cmd->Draw(static_cast<uint32_t>(mDepthLines.size()) * Utils::PrimitiveTopologyVertexCount(PrimitiveTopology::Lines));
		}
		
		if (!mDepthDebugMeshes.empty())
        {
            RenderMeshes(cmd, sceneData.camera.uniforms, mDepthDebugMeshes, true);
        }

		if (!mLines.empty())
		{
			cmd->BindGraphicsPipeline(mPrimitivePipeline);
			cmd->SetConstantBuffer(resources, 0);
			cmd->SetConstantBuffer(sceneData.camera, 1);
			cmd->Draw(static_cast<uint32_t>(mLines.size()) * Utils::PrimitiveTopologyVertexCount(PrimitiveTopology::Lines));
		}

		if (!mDebugMeshes.empty())
        {
            RenderMeshes(cmd, sceneData.camera.uniforms, mDebugMeshes, false);
        }
        
        // clear after rendering
        mLines.clear();
        mDepthLines.clear();
        mDebugMeshes.clear();
        mDepthDebugMeshes.clear();
	});
}

void DebugRenderer::RenderMeshes(const CommandBuffer* cmd, const CameraUniforms& cameraData, const TArray<DebugMesh>& debugMeshes, bool depthTest) const
{
	cmd->BindGraphicsPipeline(depthTest ? mMeshDepthPipeline : mMeshPipeline);
	cmd->SetConstantBuffer(cameraData, 1);

	for (const auto& debugMesh : debugMeshes)
	{
        DebugShaderResources resources;
        resources.vertexBuffer = debugMesh.mesh->GetPositionBuffer().GetResourceView();
        cmd->SetConstantBuffer(resources, 0);
	
		for (const auto& submesh : debugMesh.mesh->GetSubmeshes())
		{
			DebugMeshUniforms uniforms;
			uniforms.transform = debugMesh.transform;
			uniforms.baseVertex = submesh.baseVertex;
			uniforms.color = debugMesh.color;
			cmd->SetPushConstant(uniforms);
			cmd->DrawIndexed(debugMesh.mesh->GetIndexBuffer(), IndexType::UINT32, submesh.indexCount, 1, submesh.firstIndex);
		}
	}
}

void DebugRenderer::DrawLine(const Float3& start, const Float3& end, Color32 color, bool depthTest)
{
    DebugLine line;
    line.start = {start, color};
    line.end = {end, color};

    if (depthTest)
        mDepthLines.emplace_back(std::move(line));
    else
        mLines.emplace_back(std::move(line));
}

void DebugRenderer::DrawTriangle(const Float3& v1, const Float3& v2, const Float3& v3, Color32 color, bool depthTest)
{
	DrawLine(v1, v2, color, depthTest);
	DrawLine(v2, v3, color, depthTest);
	DrawLine(v3, v1, color, depthTest);
}

void DebugRenderer::DrawQuad(const Float3& center, float width, float height, Color32 color, bool depthTest)
{
    float halftWidth = width / 2.0f;
    float halfHeight = height / 2.0f;

    Float3 v0{ center.x - halftWidth, center.y, center.z - halfHeight };
    Float3 v1{ center.x + halftWidth, center.y, center.z - halfHeight };
    Float3 v2{ center.x + halftWidth, center.y, center.z + halfHeight };
    Float3 v3{ center.x - halftWidth, center.y, center.z + halfHeight };

    DrawLine(v0, v1, color, depthTest);
    DrawLine(v1, v2, color, depthTest);
    DrawLine(v2, v3, color, depthTest);
    DrawLine(v3, v0, color, depthTest);
}

void DebugRenderer::DrawBoundingBox(const BoundingBox& boundingBox, Color32 color, bool depthTest)
{
    const Float3& min = boundingBox.min;
    const Float3& max = boundingBox.max;

    Float3 v1(max.x, min.y, min.z);
    Float3 v2(max.x, max.y, min.z);
    Float3 v3(min.x, max.y, min.z);
    Float3 v4(min.x, min.y, max.z);
    Float3 v5(max.x, min.y, max.z);
    Float3 v6(min.x, max.y, max.z);

    DrawLine(min, v1, color, depthTest);
    DrawLine(v1, v2, color, depthTest);
    DrawLine(v2, v3, color, depthTest);
    DrawLine(v3, min, color, depthTest);
    DrawLine(v4, v5, color, depthTest);
    DrawLine(v5, max, color, depthTest);
    DrawLine(max, v6, color, depthTest);
    DrawLine(v6, v4, color, depthTest);
    DrawLine(min, v4, color, depthTest);
    DrawLine(v1, v5, color, depthTest);
    DrawLine(v2, max, color, depthTest);
    DrawLine(v3, v6, color, depthTest);
}

void DebugRenderer::DrawBoundingBox(const BoundingBox& boundingBox, const Float4x4& transform, Color32 color, bool depthTest)
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

    DrawLine(v0, v1, color, depthTest);
    DrawLine(v1, v2, color, depthTest);
    DrawLine(v2, v3, color, depthTest);
    DrawLine(v3, v0, color, depthTest);
    DrawLine(v4, v5, color, depthTest);
    DrawLine(v5, v7, color, depthTest);
    DrawLine(v7, v6, color, depthTest);
    DrawLine(v6, v4, color, depthTest);
    DrawLine(v0, v4, color, depthTest);
    DrawLine(v1, v5, color, depthTest);
    DrawLine(v2, v7, color, depthTest);
    DrawLine(v3, v6, color, depthTest);
}

void DebugRenderer::DrawMesh(const Mesh* mesh, const Float4x4& transform, Color32 color, bool depthTest)
{
	DebugMesh debugMesh;
	debugMesh.mesh = mesh;
	debugMesh.transform = transform;
	debugMesh.color = color;

    if (depthTest)
        mDepthDebugMeshes.push_back(debugMesh);
    else
        mDebugMeshes.push_back(debugMesh);
}
