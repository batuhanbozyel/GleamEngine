#include "gpch.h"
#include "DebugRenderer.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/RendererContext.h"
#include "Renderer/RendererBindingTable.h"

#include "Core/Game.h"

using namespace Gleam;

void DebugRenderer::OnCreate(RendererContext* context)
{
	mPrimitiveVertexShader = context->CreateShader("debugVertexShader", ShaderStage::Vertex);
	mMeshVertexShader = context->CreateShader("debugMeshVertexShader", ShaderStage::Vertex);
	mFragmentShader = context->CreateShader("debugFragmentShader", ShaderStage::Fragment);
}

void DebugRenderer::AddRenderPasses(RenderGraph& graph, const RenderingData& renderData)
{
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

	// nothing to render
	if (mDebugVertices.empty())
		return;

	struct UpdatePassData
	{
		BufferHandle vertexBuffer;
        BufferHandle cameraBuffer;
	};
    
	const auto& updatePass = graph.AddRenderPass<UpdatePassData>("DebugRenderer::UpdatePass", [&](RenderGraphBuilder& builder, UpdatePassData& passData)
	{
		BufferDescriptor descriptor;
		descriptor.size = mDebugVertices.size() * sizeof(DebugVertex);
		descriptor.usage = BufferUsage::VertexBuffer;
		descriptor.memoryType = MemoryType::Stream;
		passData.vertexBuffer = builder.CreateBuffer(descriptor);
		passData.vertexBuffer = builder.WriteBuffer(passData.vertexBuffer);
        
        descriptor.size = sizeof(CameraUniforms);
        descriptor.usage = BufferUsage::UniformBuffer;
        descriptor.memoryType = MemoryType::Stream;
        passData.cameraBuffer = builder.CreateBuffer(descriptor);
        passData.cameraBuffer = builder.WriteBuffer(passData.cameraBuffer);
	},
	[this](const RenderGraphContext& renderGraphContext, const UpdatePassData& passData)
	{
		// Update buffer
        auto vertexBuffer = renderGraphContext.registry->GetBuffer(passData.vertexBuffer);
        vertexBuffer->SetData(mDebugVertices.data(), mDebugVertices.size() * sizeof(DebugVertex));
        
        CameraUniforms camera;
        camera.viewMatrix = mViewMatrix;
        camera.projectionMatrix = mProjectionMatrix;
        camera.viewProjectionMatrix = mViewProjectionMatrix;
        
        auto cameraBuffer = renderGraphContext.registry->GetBuffer(passData.cameraBuffer);
        cameraBuffer->SetData(&camera, sizeof(CameraUniforms));
	});

	struct DrawPassData
	{
		RenderTextureHandle renderTarget;
		BufferHandle vertexBuffer;
        BufferHandle cameraBuffer;
	};

	graph.AddRenderPass<DrawPassData>("DebugRenderer::DrawPass", [&](RenderGraphBuilder& builder, DrawPassData& passData)
	{
        passData.vertexBuffer = builder.ReadBuffer(updatePass.vertexBuffer);
        passData.cameraBuffer = builder.ReadBuffer(updatePass.cameraBuffer);
        passData.renderTarget = builder.WriteRenderTexture(renderData.colorTarget);
	},
	[this](const RenderGraphContext& renderGraphContext, const DrawPassData& passData)
	{
		// Start rendering
        AttachmentDescriptor attachmentDesc;
        attachmentDesc.texture = renderGraphContext.registry->GetRenderTexture(passData.renderTarget);
        
        RenderPassDescriptor renderPassDesc;
        renderPassDesc.size = attachmentDesc.texture->GetDescriptor().size;
        renderPassDesc.colorAttachments.emplace_back(attachmentDesc);
        renderGraphContext.cmd->BeginRenderPass(renderPassDesc);
        
        const auto& cameraBuffer = renderGraphContext.registry->GetBuffer(passData.cameraBuffer);
        renderGraphContext.cmd->SetVertexBuffer(*cameraBuffer, 0, RendererBindingTable::CameraBuffer);
		
        const auto& vertexBuffer = renderGraphContext.registry->GetBuffer(passData.vertexBuffer);
		if (!mDepthLines.empty())
		{
            renderGraphContext.cmd->SetVertexBuffer(*vertexBuffer, mDepthLineBufferOffset, RendererBindingTable::Buffer0);
			RenderPrimitive(renderGraphContext.cmd, static_cast<uint32_t>(mDepthLines.size()), PrimitiveTopology::Lines, true);
		}

		if (!mDepthTriangles.empty())
		{
            renderGraphContext.cmd->SetVertexBuffer(*vertexBuffer, mDepthTriangleBufferOffset, RendererBindingTable::Buffer0);
			RenderPrimitive(renderGraphContext.cmd, static_cast<uint32_t>(mDepthTriangles.size()), PrimitiveTopology::Triangles, true);
		}
		
		if (!mDepthDebugMeshes.empty())
			RenderMeshes(renderGraphContext.cmd, mDepthDebugMeshes, true);

		if (!mLines.empty())
		{
            renderGraphContext.cmd->SetVertexBuffer(*vertexBuffer, mLineBufferOffset, RendererBindingTable::Buffer0);
			RenderPrimitive(renderGraphContext.cmd, static_cast<uint32_t>(mLines.size()), PrimitiveTopology::Lines, false);
		}

		if (!mTriangles.empty())
		{
            renderGraphContext.cmd->SetVertexBuffer(*vertexBuffer, mTriangleBufferOffset, RendererBindingTable::Buffer0);
			RenderPrimitive(renderGraphContext.cmd, static_cast<uint32_t>(mTriangles.size()), PrimitiveTopology::Triangles, false);
		}

		if (!mDebugMeshes.empty())
			RenderMeshes(renderGraphContext.cmd, mDebugMeshes, false);
        
        renderGraphContext.cmd->EndRenderPass();
        
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

void DebugRenderer::RenderPrimitive(const CommandBuffer* cmd, uint32_t primitiveCount, PrimitiveTopology topology, bool depthTest) const
{
	PipelineStateDescriptor pipelineState;
	pipelineState.topology = topology;
	pipelineState.depthState.writeEnabled = depthTest;
	cmd->BindGraphicsPipeline(pipelineState, mPrimitiveVertexShader, mFragmentShader);

    cmd->Draw(primitiveCount * Utils::PrimitiveTopologyVertexCount(topology));
}

void DebugRenderer::RenderMeshes(const CommandBuffer* cmd, const TArray<DebugMesh>& debugMeshes, bool depthTest) const
{
	PipelineStateDescriptor pipelineState;
	pipelineState.topology = PrimitiveTopology::Triangles;
	pipelineState.depthState.writeEnabled = depthTest;
	cmd->BindGraphicsPipeline(pipelineState, mMeshVertexShader, mFragmentShader);

	for (const auto& debugMesh : debugMeshes)
	{
		const auto& meshBuffer = debugMesh.mesh->GetBuffer();
		cmd->SetVertexBuffer(*meshBuffer.GetPositionBuffer(), 0, RendererBindingTable::Buffer0);

		DebugShaderUniforms uniforms;
		uniforms.modelMatrix = debugMesh.transform;
		uniforms.color = debugMesh.color;
		cmd->SetPushConstant(uniforms, ShaderStage_Vertex | ShaderStage_Fragment);
	
		for (const auto& submesh : debugMesh.mesh->GetSubmeshDescriptors())
			cmd->DrawIndexed(meshBuffer.GetIndexBuffer()->GetHandle(), IndexType::UINT32, submesh.indexCount, 1, submesh.firstIndex, submesh.baseVertex, 0);
	}	
}

void DebugRenderer::DrawLine(const Vector3& start, const Vector3& end, Color32 color, bool depthTest)
{
    DebugLine line;
    line.start = {start, color};
    line.end = {end, color};

    if (depthTest)
        mDepthLines.emplace_back(std::move(line));
    else
        mLines.emplace_back(std::move(line));
}

void DebugRenderer::DrawTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3, Color32 color, bool depthTest)
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

void DebugRenderer::DrawQuad(const Vector3& center, float width, float height, Color32 color, bool depthTest)
{
    float halftWidth = width / 2.0f;
    float halfHeight = height / 2.0f;

    Vector3 v0{ center.x - halftWidth, center.y, center.z - halfHeight };
    Vector3 v1{ center.x + halftWidth, center.y, center.z - halfHeight };
    Vector3 v2{ center.x + halftWidth, center.y, center.z + halfHeight };
    Vector3 v3{ center.x - halftWidth, center.y, center.z + halfHeight };

    DrawLine(v0, v1, color, depthTest);
    DrawLine(v1, v2, color, depthTest);
    DrawLine(v2, v3, color, depthTest);
    DrawLine(v3, v0, color, depthTest);
}

void DebugRenderer::DrawBoundingBox(const BoundingBox& boundingBox, Color32 color, bool depthTest)
{
    const Vector3& min = boundingBox.min;
    const Vector3& max = boundingBox.max;

    Vector3 v1(max.x, min.y, min.z);
    Vector3 v2(max.x, max.y, min.z);
    Vector3 v3(min.x, max.y, min.z);
    Vector3 v4(min.x, min.y, max.z);
    Vector3 v5(max.x, min.y, max.z);
    Vector3 v6(min.x, max.y, max.z);

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

void DebugRenderer::DrawBoundingBox(const BoundingBox& boundingBox, const Matrix4& transform, Color32 color, bool depthTest)
{
    const Vector3& min = boundingBox.min;
    const Vector3& max = boundingBox.max;

    Vector3 v0(transform * min);
    Vector3 v1(transform * Vector3(max.x, min.y, min.z));
    Vector3 v2(transform * Vector3(max.x, max.y, min.z));
    Vector3 v3(transform * Vector3(min.x, max.y, min.z));
    Vector3 v4(transform * Vector3(min.x, min.y, max.z));
    Vector3 v5(transform * Vector3(max.x, min.y, max.z));
    Vector3 v6(transform * Vector3(min.x, max.y, max.z));
    Vector3 v7(transform * max);

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

void DebugRenderer::DrawMesh(const Mesh* mesh, const Matrix4& transform, Color32 color, bool depthTest)
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

void DebugRenderer::UpdateCamera(const Camera& camera)
{
    mViewMatrix = camera.GetViewMatrix();
    mProjectionMatrix = camera.GetProjectionMatrix();
    mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
}
