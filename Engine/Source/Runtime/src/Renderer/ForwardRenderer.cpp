#include "gpch.h"
#include "ForwardRenderer.h"
#include "RendererContext.h"
#include "ShaderLibrary.h"
#include "RenderPassDescriptor.h"
#include "PipelineStateDescriptor.h"

using namespace Gleam;

ForwardRenderer::ForwardRenderer()
    : mVertexBuffer(4), mIndexBuffer(6, IndexType::UINT16)
{
    TArray<MeshVertex, 4> vertices;
    // top left
    vertices[0].position = { -0.5f, 0.5f, 0.0f };
    vertices[0].texCoord = { 0.0f, 0.0f };
    // top right
    vertices[1].position = { 0.5f, 0.5f, 0.0f };
    vertices[1].texCoord = { 1.0f, 0.0f };
    // bottom right
    vertices[2].position = { 0.5f, -0.5f, 0.0f };
    vertices[2].texCoord = { 1.0f, 1.0f };
    // bottom left
    vertices[3].position = { -0.5f, -0.5f, 0.0f };
    vertices[3].texCoord = { 0.0f, 1.0f };
    mVertexBuffer.SetData<4>(vertices);

    TArray<uint16_t, 6> indices
    {
        0, 1, 2,
        2, 3, 0
    };
    mIndexBuffer.SetData<6>(indices);
    
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
	mCommandBuffer.SetVertexBuffer(mVertexBuffer);
    
    ForwardPassFragmentUniforms uniforms;
    uniforms.color = Color::HSVToRGB(static_cast<float>(Time::time), 1.0f, 1.0f);
	mCommandBuffer.SetPushConstant(uniforms, ShaderStage::Fragment);
    
	mCommandBuffer.DrawIndexed(mIndexBuffer);
	mCommandBuffer.EndRenderPass();
	mCommandBuffer.End();

	mCommandBuffer.Commit();
}
