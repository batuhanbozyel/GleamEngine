#include "gpch.h"
#include "ForwardRenderer.h"
#include "RendererContext.h"
#include "ShaderLibrary.h"
#include "RenderPassDescriptor.h"
#include "PipelineStateDescriptor.h"

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
	renderPassDesc.swapchainTarget = true;
	renderPassDesc.width = static_cast<uint32_t>(drawableSize.x);
	renderPassDesc.height = static_cast<uint32_t>(drawableSize.y);
    
	mCommandBuffer.Begin();
	mCommandBuffer.BeginRenderPass(renderPassDesc);
    mCommandBuffer.BindPipeline(PipelineStateDescriptor(), mForwardPassProgram);
	mCommandBuffer.SetViewport(renderPassDesc.width, renderPassDesc.height);
    
    for (const auto& [mesh, transform] : mMeshes)
    {
        const auto& meshBuffer = mesh->GetBuffer();
        mCommandBuffer.SetVertexBuffer(meshBuffer.GetPositionBuffer());
        mCommandBuffer.SetVertexBuffer(meshBuffer.GetInterleavedBuffer(), 1);
        
        CameraUniforms cameraUniforms;
        cameraUniforms.modelMatrix = transform;
        cameraUniforms.viewMatrix = mViewMatrix;
        cameraUniforms.projectionMatrix = mProjectionMatrix;
        cameraUniforms.viewProjectionMatrix = mProjectionMatrix * mViewMatrix;
        cameraUniforms.modelViewProjectionMatrix = mProjectionMatrix * mViewMatrix * transform;
        cameraUniforms.normalMatrix = Matrix3::identity; // TODO:
        mCommandBuffer.SetPushConstant(cameraUniforms, ShaderStage::Vertex, 2);
        
        ForwardPassFragmentUniforms fragmentUniforms;
        fragmentUniforms.color = Color::HSVToRGB(static_cast<float>(Time::time), 1.0f, 1.0f);
        mCommandBuffer.SetPushConstant(fragmentUniforms, ShaderStage::Fragment);
        
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
