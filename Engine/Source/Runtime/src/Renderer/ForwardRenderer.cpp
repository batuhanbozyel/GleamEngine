#include "gpch.h"
#include "ForwardRenderer.h"
#include "RendererContext.h"
#include "ShaderLibrary.h"
#include "RenderPassDescriptor.h"
#include "PipelineStateDescriptor.h"
#include "RendererBindingTable.h"

using namespace Gleam;

ForwardRenderer::ForwardRenderer()
{
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
    renderPassDesc.size = drawableSize;

    mCommandBuffer.Begin();
    mCommandBuffer.BeginRenderPass(renderPassDesc);
    mCommandBuffer.BindPipeline(PipelineStateDescriptor(), mForwardPassProgram);
    mCommandBuffer.SetViewport(renderPassDesc.size);

    for (const auto& [mesh, transform] : mMeshes)
    {
        const auto& meshBuffer = mesh->GetBuffer();
        mCommandBuffer.SetVertexBuffer(meshBuffer.GetPositionBuffer(), RendererBindingTable::Buffer0);
        mCommandBuffer.SetVertexBuffer(meshBuffer.GetInterleavedBuffer(), RendererBindingTable::Buffer1);
        mCommandBuffer.SetVertexBuffer(GetCameraBuffer(), ShaderStage_Vertex);
        
        ForwardPassUniforms uniforms;
        uniforms.color = Color::HSVToRGB(static_cast<float>(Time::time), 1.0f, 1.0f);
        mCommandBuffer.SetPushConstant(uniforms, ShaderStage_Vertex | ShaderStage_Fragment);
        
        for (const auto& submesh : mesh->GetSubmeshDescriptors())
        {
            mCommandBuffer.DrawIndexed(meshBuffer.GetIndexBuffer(), submesh.indexCount, 1, submesh.firstIndex, submesh.baseVertex);
        }
    }

    mCommandBuffer.EndRenderPass();
    mCommandBuffer.End();
    mCommandBuffer.Commit();

    // reset render queue
    mMeshes.clear();
}

void ForwardRenderer::DrawMesh(const Mesh* mesh, const Matrix4& transform)
{
    mMeshes.push_back({mesh, transform});
}
