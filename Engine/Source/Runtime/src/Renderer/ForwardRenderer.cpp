#include "gpch.h"
#include "ForwardRenderer.h"
#include "RendererContext.h"
#include "ShaderLibrary.h"
#include "RenderPassDescriptor.h"
#include "PipelineStateDescriptor.h"

using namespace Gleam;

ForwardRenderer::ForwardRenderer()
    : mPositionBuffer(4), mInterleavedBuffer(4), mIndexBuffer(6, IndexType::UINT16)
{
    TArray<Vector3, 4> positions;
    TArray<InterleavedMeshVertex, 4> interleavedVertices;
    // top left
    positions[0] = { -0.5f, 0.5f, 0.0f };
    interleavedVertices[0].texCoord = { 0.0f, 0.0f };
    // top right
    positions[1] = { 0.5f, 0.5f, 0.0f };
    interleavedVertices[1].texCoord = { 1.0f, 0.0f };
    // bottom right
    positions[2] = { 0.5f, -0.5f, 0.0f };
    interleavedVertices[2].texCoord = { 1.0f, 1.0f };
    // bottom left
    positions[3] = { -0.5f, -0.5f, 0.0f };
    interleavedVertices[3].texCoord = { 0.0f, 1.0f };
    mPositionBuffer.SetData(positions);
    mInterleavedBuffer.SetData(interleavedVertices);

    TArray<uint16_t, 6> indices
    {
        0, 1, 2,
        2, 3, 0
    };
    mIndexBuffer.SetData(indices);
    
    mForwardPassProgram.vertexShader = ShaderLibrary::CreateShader("forwardPassVertexShader", ShaderStage::Vertex);
	mForwardPassProgram.fragmentShader = ShaderLibrary::CreateShader("forwardPassFragmentShader", ShaderStage::Fragment);
}

ForwardRenderer::~ForwardRenderer()
{
	mCommandBuffer.WaitUntilCompleted();
}

void ForwardRenderer::Render()
{
	const auto& drawableSize = GetDrawableSize();
    RenderPassDescriptor renderPassDesc;
	renderPassDesc.swapchainTarget = true;
	renderPassDesc.width = static_cast<uint32_t>(drawableSize.x);
	renderPassDesc.height = static_cast<uint32_t>(drawableSize.y);
    
	mCommandBuffer.Begin();
	mCommandBuffer.BeginRenderPass(renderPassDesc);
    mCommandBuffer.BindPipeline(PipelineStateDescriptor(), mForwardPassProgram);
    
	mCommandBuffer.SetViewport(renderPassDesc.width, renderPassDesc.height);
	mCommandBuffer.SetVertexBuffer(mPositionBuffer, 0);
    mCommandBuffer.SetVertexBuffer(mInterleavedBuffer, 1);
    
    ForwardPassFragmentUniforms uniforms;
    uniforms.color = Color::HSVToRGB(static_cast<float>(Time::time), 1.0f, 1.0f);
	mCommandBuffer.SetPushConstant(uniforms, ShaderStage::Fragment);
    
	mCommandBuffer.DrawIndexed(mIndexBuffer, mIndexBuffer.GetCount());
	mCommandBuffer.EndRenderPass();
	mCommandBuffer.End();

	mCommandBuffer.Commit();
}
