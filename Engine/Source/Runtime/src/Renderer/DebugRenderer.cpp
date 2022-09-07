#include "gpch.h"
#include "DebugRenderer.h"
#include "RendererContext.h"
#include "ShaderLibrary.h"
#include "RenderPassDescriptor.h"
#include "PipelineStateDescriptor.h"

using namespace Gleam;

static constexpr uint32_t PrimitiveTopologyVertexCount(PrimitiveTopology topology)
{
    switch (topology)
    {
        case PrimitiveTopology::Points: return 1;
        case PrimitiveTopology::Lines: return 2;
        case PrimitiveTopology::LineStrip: return 2;
        case PrimitiveTopology::Triangles: return 3;
        default: return 0;
    }
}

DebugRenderer::DebugRenderer()
	: mVertexBuffers(RendererContext::GetSwapchain()->GetProperties().maxFramesInFlight)
{
	for (auto& vertexBuffer : mVertexBuffers)
	{
		vertexBuffer = std::move(CreateScope<VertexBuffer<DebugVertex>>(100, MemoryType::Dynamic));
	}

	mDebugProgram.vertexShader = ShaderLibrary::CreateShader("debugVertexShader", ShaderStage::Vertex);
	mDebugProgram.fragmentShader = ShaderLibrary::CreateShader("debugFragmentShader", ShaderStage::Fragment);
    
    mDebugMeshProgram.vertexShader = ShaderLibrary::CreateShader("debugMeshVertexShader", ShaderStage::Vertex);
    mDebugMeshProgram.fragmentShader = ShaderLibrary::CreateShader("debugFragmentShader", ShaderStage::Fragment);
    
    Camera defaultCamera(Renderer::GetDrawableSize());
    mViewMatrix = defaultCamera.GetViewMatrix();
    mProjectionMatrix = defaultCamera.GetProjectionMatrix();
}

DebugRenderer::~DebugRenderer()
{
	mCommandBuffer.WaitUntilCompleted();
}

void DebugRenderer::Render()
{
    if (mLines.empty() &&
        mDepthLines.empty() &&
        mTriangles.empty() &&
        mDepthTriangles.empty())
    {
        return;
    }
    
    // Prepare data for rendering
    for (const auto& line : mLines)
    {
        mStagingBuffer.push_back(line.start);
        mStagingBuffer.push_back(line.end);
    }
    
    uint32_t triangleBufferOffset = static_cast<uint32_t>(mStagingBuffer.size() * sizeof(DebugVertex));
    for (const auto& triangle : mTriangles)
    {
        mStagingBuffer.push_back(triangle.vertex1);
        mStagingBuffer.push_back(triangle.vertex2);
        mStagingBuffer.push_back(triangle.vertex3);
    }
    
    uint32_t depthLineBufferOffset = static_cast<uint32_t>(mStagingBuffer.size() * sizeof(DebugVertex));
    for (const auto& line : mDepthLines)
    {
        mStagingBuffer.push_back(line.start);
        mStagingBuffer.push_back(line.end);
    }
    
    uint32_t depthTriangleBufferOffset = static_cast<uint32_t>(mStagingBuffer.size() * sizeof(DebugVertex));
    for (const auto& triangle : mDepthTriangles)
    {
        mStagingBuffer.push_back(triangle.vertex1);
        mStagingBuffer.push_back(triangle.vertex2);
        mStagingBuffer.push_back(triangle.vertex3);
    }
    
    // Update buffer
	auto& vertexBuffer = mVertexBuffers[RendererContext::GetSwapchain()->GetFrameIndex()];
    if (vertexBuffer->GetCount() < mStagingBuffer.size())
    {
		vertexBuffer->Resize(static_cast<uint32_t>(mStagingBuffer.size()));
    }
	vertexBuffer->SetData(mStagingBuffer);
    
    // Start rendering
	const auto& drawableSize = GetDrawableSize();
    RenderPassDescriptor renderPassDesc;
    renderPassDesc.swapchainTarget = true;
    renderPassDesc.width = static_cast<uint32_t>(drawableSize.x);
    renderPassDesc.height = static_cast<uint32_t>(drawableSize.y);
    
    mCommandBuffer.Begin();
    mCommandBuffer.BeginRenderPass(renderPassDesc);
    mCommandBuffer.SetViewport(renderPassDesc.width, renderPassDesc.height);
    
    if (!mLines.empty())
    {
        RenderPrimitive(static_cast<uint32_t>(mLines.size()), 0, PrimitiveTopology::Lines, false);
    }
    
    if (!mTriangles.empty())
    {
        RenderPrimitive(static_cast<uint32_t>(mTriangles.size()), triangleBufferOffset, PrimitiveTopology::Triangles, false);
    }
    
    if (!mDepthLines.empty())
    {
        RenderPrimitive(static_cast<uint32_t>(mDepthLines.size()), depthLineBufferOffset, PrimitiveTopology::Lines, true);
    }
    
    if (!mDepthTriangles.empty())
    {
        RenderPrimitive(static_cast<uint32_t>(mDepthTriangles.size()), depthTriangleBufferOffset, PrimitiveTopology::Triangles, true);
    }
    
    for (const auto& [mesh, transform, color] : mMeshes)
    {
        RenderMesh(mesh->GetBuffer(), mesh->GetSubmeshDescriptors(), transform, color, false);
    }
    
    for (const auto& [mesh, transform, color] : mDepthMeshes)
    {
        RenderMesh(mesh->GetBuffer(), mesh->GetSubmeshDescriptors(), transform, color, true);
    }
    
    mCommandBuffer.EndRenderPass();
    mCommandBuffer.End();
    mCommandBuffer.Commit();
    
	// reset render queue
	mLines.clear();
	mDepthLines.clear();
	mTriangles.clear();
	mDepthTriangles.clear();
    mStagingBuffer.clear();
    mMeshes.clear();
    mDepthMeshes.clear();
	mSkeletalMeshes.clear();
	mDepthSkeletalMeshes.clear();
}

void DebugRenderer::RenderPrimitive(uint32_t primitiveCount, uint32_t bufferOffset, PrimitiveTopology topology, bool depthTest) const
{
	const auto& vertexBuffer = mVertexBuffers[RendererContext::GetSwapchain()->GetFrameIndex()];

    PipelineStateDescriptor pipelineDesc;
    pipelineDesc.topology = topology;
    
    mCommandBuffer.BindPipeline(pipelineDesc, mDebugProgram);
    mCommandBuffer.SetVertexBuffer(*vertexBuffer, 0, bufferOffset);
    
    CameraUniforms cameraUniforms;
    cameraUniforms.modelMatrix = Matrix4::identity;
    cameraUniforms.viewMatrix = mViewMatrix;
    cameraUniforms.projectionMatrix = mProjectionMatrix;
    cameraUniforms.viewProjectionMatrix = mProjectionMatrix * mViewMatrix;
    cameraUniforms.modelViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
    cameraUniforms.normalMatrix = Matrix3::identity; // TODO:
    mCommandBuffer.SetPushConstant(cameraUniforms, ShaderStage::Vertex, 1);
    mCommandBuffer.Draw(primitiveCount * PrimitiveTopologyVertexCount(topology));
}

void DebugRenderer::RenderMesh(const MeshBuffer& meshBuffer, const TArray<SubmeshDescriptor>& submeshDescriptors, const Matrix4& transform, Color32 color, bool depthTest) const
{
    PipelineStateDescriptor pipelineDesc;
    mCommandBuffer.BindPipeline(pipelineDesc, mDebugMeshProgram);
    mCommandBuffer.SetVertexBuffer(meshBuffer.GetPositionBuffer());
    
    CameraUniforms cameraUniforms;
    cameraUniforms.modelMatrix = transform;
    cameraUniforms.viewMatrix = mViewMatrix;
    cameraUniforms.projectionMatrix = mProjectionMatrix;
    cameraUniforms.viewProjectionMatrix = mProjectionMatrix * mViewMatrix;
    cameraUniforms.modelViewProjectionMatrix = mProjectionMatrix * mViewMatrix * transform;
    cameraUniforms.normalMatrix = Matrix3::identity; // TODO:
    mCommandBuffer.SetPushConstant(cameraUniforms, ShaderStage::Vertex, 1);
    
    DebugFragmentUniforms fragmentUniforms;
    fragmentUniforms.color = color;
    mCommandBuffer.SetPushConstant(fragmentUniforms, ShaderStage::Fragment);
    
    for (const auto& submesh : submeshDescriptors)
    {
        mCommandBuffer.DrawIndexed(meshBuffer.GetIndexBuffer(), submesh.indexCount, 1, submesh.firstIndex, submesh.baseVertex);
    }
}

void DebugRenderer::DrawLine(const Vector3& start, const Vector3& end, Color32 color, bool depthTest)
{
	DebugLine line;
    line.start = {start, color};
    line.end = {end, color};
    
	if (depthTest)
	{
		mDepthLines.emplace_back(std::move(line));
	}
	else
	{
		mLines.emplace_back(std::move(line));
	}
}

void DebugRenderer::DrawTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3, Color32 color, bool depthTest)
{
	DebugTriangle triangle;
    triangle.vertex1 = {v1, color};
    triangle.vertex2 = {v2, color};
    triangle.vertex3 = {v3, color};

	if (depthTest)
	{
		mDepthTriangles.emplace_back(std::move(triangle));
	}
	else
	{
		mTriangles.emplace_back(std::move(triangle));
	}
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

void DebugRenderer::DrawMesh(const Mesh* mesh, const Matrix4& transform, Color32 color, bool depthTest)
{
    if (depthTest)
    {
        mDepthMeshes.push_back({mesh, transform, color});
    }
    else
    {
        mMeshes.push_back({mesh, transform, color});
    }
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
