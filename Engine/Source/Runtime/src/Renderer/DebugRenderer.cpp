#include "gpch.h"
#include "DebugRenderer.h"
#include "ShaderLibrary.h"
#include "RendererContext.h"
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
	: mVertexBuffer(100)
{
	mDebugProgram.vertexShader = ShaderLibrary::CreateShader("debugVertexShader", ShaderStage::Vertex);
	mDebugProgram.fragmentShader = ShaderLibrary::CreateShader("debugFragmentShader", ShaderStage::Fragment);
}

DebugRenderer::~DebugRenderer()
{
    
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
    
    uint32_t triangleBufferOffset = mStagingBuffer.size();
    for (const auto& triangle : mTrianlges)
    {
        mStagingBuffer.push_back(triangle.vertex1);
        mStagingBuffer.push_back(triangle.vertex2);
        mStagingBuffer.push_back(triangle.vertex3);
    }
    
    uint32_t depthLineBufferOffset = mStagingBuffer.size();
    for (const auto& line : mDepthLines)
    {
        mStagingBuffer.push_back(line.start);
        mStagingBuffer.push_back(line.end);
    }
    
    uint32_t depthTriangleBufferOffset = mStagingBuffer.size();
    for (const auto& triangle : mDepthTrianlges)
    {
        mStagingBuffer.push_back(triangle.vertex1);
        mStagingBuffer.push_back(triangle.vertex2);
        mStagingBuffer.push_back(triangle.vertex3);
    }
    
    // Update buffer
    if (mVertexBuffer.GetCount() < mStagingBuffer.size())
    {
        mVertexBuffer.Resize(mStagingBuffer.size());
    }
    mVertexBuffer.SetData(mStagingBuffer);
    
    // Start rendering
    mCommandBuffer.Begin();
    if (!mLines.empty())
    {
        RenderPrimitive(mLines.size(), 0, PrimitiveTopology::Lines, false);
    }
    
    if (!mTrianlges.empty())
    {
        RenderPrimitive(mTrianlges.size(), triangleBufferOffset, PrimitiveTopology::Triangles, false);
    }
    
    if (!mDepthLines.empty())
    {
        RenderPrimitive(mDepthLines.size(), depthLineBufferOffset, PrimitiveTopology::Lines, true);
    }
    
    if (!mDepthTrianlges.empty())
    {
        RenderPrimitive(mDepthTrianlges.size(), depthTriangleBufferOffset, PrimitiveTopology::Triangles, true);
    }
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
    RenderPassDescriptor renderPassDesc;
    renderPassDesc.swapchainTarget = true;
    renderPassDesc.width = RendererContext::GetProperties().width;
    renderPassDesc.height = RendererContext::GetProperties().height;
    
    PipelineStateDescriptor pipelineDesc;
    pipelineDesc.topology = topology;
    
    mCommandBuffer.BeginRenderPass(renderPassDesc, pipelineDesc, mDebugProgram);
    
    mCommandBuffer.SetViewport(RendererContext::GetProperties().width, RendererContext::GetProperties().height);
    mCommandBuffer.SetVertexBuffer(mVertexBuffer, 0, bufferOffset);
    
    mCommandBuffer.Draw(primitiveCount * PrimitiveTopologyVertexCount(topology));
    mCommandBuffer.EndRenderPass();
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
