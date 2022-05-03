#include "gpch.h"
#include "ForwardRenderer.h"
#include "RendererContext.h"
#include "ShaderLibrary.h"
#include "RenderPassDescriptor.h"
#include "PipelineStateDescriptor.h"

using namespace Gleam;

ForwardRenderer::ForwardRenderer()
    : mVertexBuffer(4), mIndexBuffer(6, IndexType::UINT32)
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
    mVertexBuffer.SetData(vertices.data(), 0, sizeof(Vertex) * 4);

    TArray<uint32_t, 6> indices
    {
        0, 1, 2,
        2, 3, 0
    };
    mIndexBuffer.SetData(indices.data(), 0, sizeof(uint32_t) * 6);
    
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
    
    static float red = 0.0f, green = 0.0f, blue = 0.0f;
    red += 0.25f * Time::deltaTime; green += 0.3f * Time::deltaTime; blue += 0.35f * Time::deltaTime;
    ForwardPassFragmentUniforms uniforms;
    uniforms.color = Color(std::fmod(red, 1.0f), std::fmod(green, 1.0f), std::fmod(blue, 1.0f), 1.0f);
    commandBuffer.SetPushConstant(uniforms, ShaderStage::Fragment);
    
    commandBuffer.DrawIndexed(mIndexBuffer);
    commandBuffer.EndRenderPass();
}
