#include "gpch.h"
#include "DebugRenderer.h"
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
	: mVertexBuffer(100, MemoryType::Dynamic)
{
	mDebugProgram.vertexShader = ShaderLibrary::CreateShader("debugVertexShader", ShaderStage::Vertex);
	mDebugProgram.fragmentShader = ShaderLibrary::CreateShader("debugFragmentShader", ShaderStage::Fragment);
    
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
        mTrianlges.empty() &&
        mDepthTrianlges.empty())
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
    for (const auto& triangle : mTrianlges)
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
    for (const auto& triangle : mDepthTrianlges)
    {
        mStagingBuffer.push_back(triangle.vertex1);
        mStagingBuffer.push_back(triangle.vertex2);
        mStagingBuffer.push_back(triangle.vertex3);
    }
    
    // Update buffer
    if (mVertexBuffer.GetCount() < mStagingBuffer.size())
    {
        mVertexBuffer.Resize(static_cast<uint32_t>(mStagingBuffer.size()));
    }
    mVertexBuffer.SetData(mStagingBuffer);
    
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
    
    if (!mTrianlges.empty())
    {
        RenderPrimitive(static_cast<uint32_t>(mTrianlges.size()), triangleBufferOffset, PrimitiveTopology::Triangles, false);
    }
    
    if (!mDepthLines.empty())
    {
        RenderPrimitive(static_cast<uint32_t>(mDepthLines.size()), depthLineBufferOffset, PrimitiveTopology::Lines, true);
    }
    
    if (!mDepthTrianlges.empty())
    {
        RenderPrimitive(static_cast<uint32_t>(mDepthTrianlges.size()), depthTriangleBufferOffset, PrimitiveTopology::Triangles, true);
    }
    mCommandBuffer.EndRenderPass();
    mCommandBuffer.End();
    mCommandBuffer.Commit();
    
	// reset render queue
	mLines.clear();
	mDepthLines.clear();
	mTrianlges.clear();
	mDepthTrianlges.clear();
    mStagingBuffer.clear();
}

void DebugRenderer::RenderPrimitive(uint32_t primitiveCount, uint32_t bufferOffset, PrimitiveTopology topology, bool depthTest) const
{
    PipelineStateDescriptor pipelineDesc;
    pipelineDesc.topology = topology;
    
    mCommandBuffer.BindPipeline(pipelineDesc, mDebugProgram);
    mCommandBuffer.SetVertexBuffer(mVertexBuffer, 0, bufferOffset);
    
    DebugVertexUniforms uniforms;
    uniforms.viewMatrix = mViewMatrix;
    uniforms.projectionMatrix = mProjectionMatrix;
    uniforms.viewProjectionMatrix = mProjectionMatrix * mViewMatrix;
    mCommandBuffer.SetPushConstant(uniforms, ShaderStage::Vertex, 1);
    mCommandBuffer.Draw(primitiveCount * PrimitiveTopologyVertexCount(topology));
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
		mDepthTrianlges.emplace_back(std::move(triangle));
	}
	else
	{
		mTrianlges.emplace_back(std::move(triangle));
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
