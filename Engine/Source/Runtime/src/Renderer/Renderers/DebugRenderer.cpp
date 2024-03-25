#include "gpch.h"
#include "DebugRenderer.h"
#include "WorldRenderer.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

using namespace Gleam;

void DebugRenderer::OnCreate(GraphicsDevice* device)
{
	mPrimitiveVertexShader = device->CreateShader("debugVertexShader", ShaderStage::Vertex);
	mMeshVertexShader = device->CreateShader("debugMeshVertexShader", ShaderStage::Vertex);
	mFragmentShader = device->CreateShader("debugFragmentShader", ShaderStage::Fragment);
}

void DebugRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	size_t vertexCount = mLines.size() + mTriangles.size() + mDepthLines.size() + mDepthTriangles.size();
	mDebugVertices.reserve(vertexCount);

	// nothing to render
	if (vertexCount == 0 && mDebugMeshes.empty())
		return;

	for (const auto& line : mLines)
	{
		mDebugVertices.push_back(line.start);
		mDebugVertices.push_back(line.end);
	}

	mTriangleBufferOffset = static_cast<uint32_t>(mDebugVertices.size() * sizeof(DebugVertex));
	for (const auto& triangle : mTriangles)
	{
		mDebugVertices.push_back(triangle.vertex1);
		mDebugVertices.push_back(triangle.vertex2);
		mDebugVertices.push_back(triangle.vertex3);
	}

	mDepthLineBufferOffset = static_cast<uint32_t>(mDebugVertices.size() * sizeof(DebugVertex));
	for (const auto& line : mDepthLines)
	{
		mDebugVertices.push_back(line.start);
		mDebugVertices.push_back(line.end);
	}

	mDepthTriangleBufferOffset = static_cast<uint32_t>(mDebugVertices.size() * sizeof(DebugVertex));
	for (const auto& triangle : mDepthTriangles)
	{
		mDebugVertices.push_back(triangle.vertex1);
		mDebugVertices.push_back(triangle.vertex2);
		mDebugVertices.push_back(triangle.vertex3);
	}

	struct UpdatePassData
	{
		BufferHandle vertexBuffer;
	};
    
	const auto& updatePass = graph.AddRenderPass<UpdatePassData>("DebugRenderer::UpdatePass", [&](RenderGraphBuilder& builder, UpdatePassData& passData)
	{
		passData.vertexBuffer = builder.CreateBuffer(mDebugVertices.size() * sizeof(DebugVertex));
		passData.vertexBuffer = builder.WriteBuffer(passData.vertexBuffer);
	},
	[this](const CommandBuffer* cmd, const UpdatePassData& passData)
	{
        cmd->SetBufferData(passData.vertexBuffer, mDebugVertices.data(), mDebugVertices.size() * sizeof(DebugVertex));
	});

	struct DrawPassData
	{
		TextureHandle colorTarget;
		TextureHandle depthTarget;
		BufferHandle vertexBuffer;
        BufferHandle cameraBuffer;
	};

	graph.AddRenderPass<DrawPassData>("DebugRenderer::DrawPass", [&](RenderGraphBuilder& builder, DrawPassData& passData)
	{
        const auto& sceneData = blackboard.Get<SceneRenderingData>();
        auto& worldData = blackboard.Get<WorldRenderingData>();
        passData.colorTarget = builder.UseColorBuffer(worldData.colorTarget);
        passData.depthTarget = builder.UseDepthBuffer(worldData.depthTarget);
        passData.vertexBuffer = builder.ReadBuffer(updatePass.vertexBuffer);
        passData.cameraBuffer = builder.ReadBuffer(sceneData.cameraBuffer);
        
        worldData.colorTarget = passData.colorTarget;
        worldData.depthTarget = passData.depthTarget;
	},
	[this](const CommandBuffer* cmd, const DrawPassData& passData)
	{
        DebugShaderResources resources;
        resources.vertexBuffer = passData.vertexBuffer;
        resources.cameraBuffer = passData.cameraBuffer;
        cmd->SetConstantBuffer(resources, 0);
        
		if (!mDepthLines.empty())
		{
			PipelineStateDescriptor pipelineState;
			pipelineState.topology = PrimitiveTopology::Lines;
			pipelineState.depthState.writeEnabled = true;
			pipelineState.depthState.compareFunction = CompareFunction::Less;
			cmd->BindGraphicsPipeline(pipelineState, mPrimitiveVertexShader, mFragmentShader);
			cmd->Draw(static_cast<uint32_t>(mDepthLines.size()) * Utils::PrimitiveTopologyVertexCount(pipelineState.topology));
		}

		if (!mDepthTriangles.empty())
		{
			PipelineStateDescriptor pipelineState;
			pipelineState.topology = PrimitiveTopology::Triangles;
			pipelineState.depthState.writeEnabled = true;
			pipelineState.depthState.compareFunction = CompareFunction::Less;
			cmd->BindGraphicsPipeline(pipelineState, mPrimitiveVertexShader, mFragmentShader);
			cmd->Draw(static_cast<uint32_t>(mDepthTriangles.size()) * Utils::PrimitiveTopologyVertexCount(pipelineState.topology));
		}
		
		if (!mDepthDebugMeshes.empty())
        {
            RenderMeshes(cmd, passData.cameraBuffer, mDepthDebugMeshes, true);
        }

		if (!mLines.empty())
		{
			PipelineStateDescriptor pipelineState;
			pipelineState.topology = PrimitiveTopology::Lines;
			pipelineState.depthState.compareFunction = CompareFunction::Less;
			cmd->BindGraphicsPipeline(pipelineState, mPrimitiveVertexShader, mFragmentShader);
			cmd->Draw(static_cast<uint32_t>(mLines.size()) * Utils::PrimitiveTopologyVertexCount(pipelineState.topology));
		}

		if (!mTriangles.empty())
		{
			PipelineStateDescriptor pipelineState;
			pipelineState.topology = PrimitiveTopology::Triangles;
			cmd->BindGraphicsPipeline(pipelineState, mPrimitiveVertexShader, mFragmentShader);
			cmd->Draw(static_cast<uint32_t>(mTriangles.size()) * Utils::PrimitiveTopologyVertexCount(pipelineState.topology));
		}

		if (!mDebugMeshes.empty())
        {
            RenderMeshes(cmd, passData.cameraBuffer, mDebugMeshes, false);
        }
        
        // clear after rendering
        mLines.clear();
        mDepthLines.clear();
        mTriangles.clear();
        mDepthTriangles.clear();
        mDebugVertices.clear();
        mDebugMeshes.clear();
        mDepthDebugMeshes.clear();
	});
}

void DebugRenderer::RenderMeshes(const CommandBuffer* cmd, const BufferHandle& cameraBuffer, const TArray<DebugMesh>& debugMeshes, bool depthTest) const
{
	PipelineStateDescriptor pipelineState;
	pipelineState.topology = PrimitiveTopology::Triangles;
	pipelineState.depthState.writeEnabled = depthTest;
	pipelineState.depthState.compareFunction = depthTest ? CompareFunction::Less : CompareFunction::Always;
	cmd->BindGraphicsPipeline(pipelineState, mMeshVertexShader, mFragmentShader);

	for (const auto& debugMesh : debugMeshes)
	{
        DebugShaderResources resources;
        resources.vertexBuffer = debugMesh.mesh->GetPositionBuffer().GetResourceView();
        resources.cameraBuffer = cameraBuffer;
        cmd->SetConstantBuffer(resources, 0);
	
		for (const auto& submesh : debugMesh.mesh->GetSubmeshDescriptors())
		{
			DebugMeshUniforms uniforms;
			uniforms.modelMatrix = debugMesh.transform;
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
    DebugTriangle triangle;
    triangle.vertex1 = {v1, color};
    triangle.vertex2 = {v2, color};
    triangle.vertex3 = {v3, color};

    if (depthTest)
        mDepthTriangles.emplace_back(std::move(triangle));
    else
        mTriangles.emplace_back(std::move(triangle));
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
