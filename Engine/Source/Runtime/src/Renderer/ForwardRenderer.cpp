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
    TArray<Vertex, 4> vertices;
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
    mVertexBuffer.SetData(vertices.data(), 0, vertices.size());

    TArray<uint16_t, 6> indices
    {
        0, 1, 2,
        2, 3, 0
    };
    mIndexBuffer.SetData(indices.data(), 0, indices.size());
    
    mForwardPassProgram = ShaderLibrary::CreateGraphicsShader("forwardPassVertexShader", "forwardPassFragmentShader");
}

ForwardRenderer::~ForwardRenderer()
{
    
}

void ForwardRenderer::Render()
{
    AttachmentDescriptor attachmentDesc;
    attachmentDesc.swapchainTarget = true;
    attachmentDesc.clearColor = mClearColor;
    
    RenderPassDescriptor renderPassDesc;
    renderPassDesc.attachments.push_back(attachmentDesc);
    
    const auto& commandBuffer = RendererContext::GetSwapchain()->GetCommandBuffer();
    commandBuffer.BeginRenderPass(renderPassDesc, PipelineStateDescriptor(), mForwardPassProgram);
    
    commandBuffer.SetViewport(RendererContext::GetProperties().width, RendererContext::GetProperties().height);
    commandBuffer.SetVertexBuffer(mVertexBuffer);
    
    ForwardPassFragmentUniforms uniforms;
    uniforms.color = Color(Color::HSVToRGBSmooth(Time::time, 1.0f, 1.0f));
    commandBuffer.SetPushConstant(uniforms, ShaderStage::Fragment);
    
    commandBuffer.DrawIndexed(mIndexBuffer);
    commandBuffer.EndRenderPass();
}
