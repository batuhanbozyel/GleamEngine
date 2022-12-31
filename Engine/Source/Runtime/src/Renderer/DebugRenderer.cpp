#include "gpch.h"
#include "DebugPass.h"
#include "Mesh.h"

using namespace Gleam;

DebugRenderer::DebugRenderer(RendererContext& context)
	: mVertexBuffer(100)
{
	mDebugPrimitiveProgram.vertexShader = context.CreateShader("debugVertexShader", ShaderStage::Vertex);
	mDebugPrimitiveProgram.fragmentShader = context.CreateShader("debugFragmentShader", ShaderStage::Fragment);

	mDebugMeshProgram.vertexShader = context.CreateShader("debugMeshVertexShader", ShaderStage::Vertex);
	mDebugMeshProgram.fragmentShader = context.CreateShader("debugFragmentShader", ShaderStage::Fragment);
}

void DebugRenderer::Configure(RendererContext& context)
{
	for (const auto& line : mLines)
    {
        mStagingBuffer.push_back(line.start);
        mStagingBuffer.push_back(line.end);
    }

    mTriangleBufferOffset = static_cast<uint32_t>(mStagingBuffer.size() * sizeof(DebugVertex));
    for (const auto& triangle : mTriangles)
    {
        mStagingBuffer.push_back(triangle.vertex1);
        mStagingBuffer.push_back(triangle.vertex2);
        mStagingBuffer.push_back(triangle.vertex3);
    }

    mDepthLineBufferOffset = static_cast<uint32_t>(mStagingBuffer.size() * sizeof(DebugVertex));
    for (const auto& line : mDepthLines)
    {
        mStagingBuffer.push_back(line.start);
        mStagingBuffer.push_back(line.end);
    }

    mDepthTriangleBufferOffset = static_cast<uint32_t>(mStagingBuffer.size() * sizeof(DebugVertex));
    for (const auto& triangle : mDepthTriangles)
    {
        mStagingBuffer.push_back(triangle.vertex1);
        mStagingBuffer.push_back(triangle.vertex2);
        mStagingBuffer.push_back(triangle.vertex3);
    }

	AttachmentDescriptor attachmentDesc;
    attachmentDesc.loadAction = AttachmentLoadAction::Clear;
    attachmentDesc.format = TextureFormat::B8G8R8A8_UNorm;
    attachmentDesc.clearColor = ApplicationInstance.backgroundColor;
	
	RenderPassDescriptor* renderPassDesc = context.CreateRenderPassDescriptor("Debug Pass");
	renderPassDesc->attachments = { attachmentDesc };
}

void DebugRenderer::Execute(CommandBuffer& cmd, const RenderingData& data)
{
    if (mLines.empty() &&
        mDepthLines.empty() &&
        mTriangles.empty() &&
        mDepthTriangles.empty())
    {
        return;
    }

    // Update buffer
    if (mVertexBuffer.GetCount() < mStagingBuffer.size())
    {
        mVertexBuffer.Resize(static_cast<uint32_t>(mStagingBuffer.size()));
    }
    mVertexBuffer.SetData(mStagingBuffer);

    // Start rendering
	cmd.SetVertexBuffer(*data.cameraBuffer, RendererBindingTable::CameraBuffer);
	
    if (!mDepthLines.empty())
	{
		cmd.SetVertexBuffer(mVertexBuffer, RendererBindingTable::Buffer0, mDepthLineBufferOffset);
		RenderPrimitive(cmd, static_cast<uint32_t>(mDepthLines.size()), PrimitiveTopology::Lines, true);
	}

    if (!mDepthTriangles.empty())
	{
		cmd.SetVertexBuffer(mVertexBuffer, RendererBindingTable::Buffer0, mDepthTriangleBufferOffset);
		RenderPrimitive(cmd, static_cast<uint32_t>(mDepthTriangles.size()), PrimitiveTopology::Triangles, true);
	}
	
    if (!mDepthDebugMeshes.empty())
        RenderMeshes(cmd, mDepthDebugMeshes, true);

    if (!mLines.empty())
	{
		cmd.SetVertexBuffer(mVertexBuffer, RendererBindingTable::Buffer0, mLineBufferOffset);
		RenderPrimitive(cmd, static_cast<uint32_t>(mLines.size()), PrimitiveTopology::Lines, false);
	}

    if (!mTriangles.empty())
	{
		cmd.SetVertexBuffer(mVertexBuffer, RendererBindingTable::Buffer0, mTriangleBufferOffset);
		RenderPrimitive(cmd, static_cast<uint32_t>(mTriangles.size()), PrimitiveTopology::Triangles, false);
	}

    if (!mDebugMeshes.empty())
        RenderMeshes(cmd, mDebugMeshes, false);
}

void DebugRenderer::Finish()
{
	mLines.clear();
    mDepthLines.clear();
    mTriangles.clear();
    mDepthTriangles.clear();
    mStagingBuffer.clear();
	mMeshes.clear();
    mDepthMeshes.clear();
}

void DebugRenderer::RenderPrimitive(const CommandBuffer& cmd, uint32_t primitiveCount, PrimitiveTopology topology, bool depthTest) const
{
	Material material;
	material.pipelineState.topology = topology;
	material.pipelineState.depthState.writeEnabled = depthTest;
	material.program = mDebugPrimitiveProgram;
	cmd.BindPipeline(material.pipelineState, material.program);

    DebugShaderUniforms uniforms;
    uniforms.modelMatrix = Matrix4::identity;
    uniforms.color = Color::white;
    cmd.SetPushConstant(uniforms, ShaderStage_Fragment);

    cmd.Draw(primitiveCount * PrimitiveTopologyVertexCount(topology));
}

void DebugRenderer::RenderMeshes(const CommandBuffer& cmd, const TArray<DebugMesh>& debugMeshes, bool depthTest) const
{
	Material material;
	material.pipelineState.topology = PrimitiveTopology::Triangles;
	material.pipelineState.depthState.writeEnabled = depthTest;
	material.program = mDebugMeshProgram;
	cmd.BindPipeline(material.pipelineState, material.program);

	for (const auto& debugMesh : debugMeshes)
	{
		const auto& meshBuffer = debugMesh.mesh->GetBuffer();
		cmd.SetVertexBuffer(meshBuffer.GetPositionBuffer(), RendererBindingTable::Buffer0);

		DebugShaderUniforms uniforms;
		uniforms.modelMatrix = transform;
		uniforms.color = color;
		cmd.SetPushConstant(uniforms, ShaderStage_Vertex | ShaderStage_Fragment);
	
		for (const auto& submesh : debugMesh.mesh->GetSubmeshDescriptors())
			cmd.DrawIndexed(meshBuffer.GetIndexBuffer(), submesh.indexCount, 1, submesh.firstIndex, submesh.baseVertex);
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
