//
//  WorldRenderer.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#include "gpch.h"
#include "WorldRenderer.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInstance.h"

using namespace Gleam;

void WorldRenderer::OnCreate(GraphicsDevice* device)
{
    mForwardPassVertexShader = device->CreateShader("forwardPassVertexShader", ShaderStage::Vertex);
    mForwardPassFragmentShader = device->CreateShader("forwardPassFragmentShader", ShaderStage::Fragment);
}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    const auto& updatePass = graph.AddRenderPass<WorldRenderingData>("WorldRenderer::UpdatePass", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
    {
        BufferDescriptor descriptor;
        descriptor.usage = BufferUsage::UniformBuffer;
        descriptor.size = sizeof(CameraUniforms);
        passData.cameraBuffer = builder.CreateBuffer(descriptor);
        passData.cameraBuffer = builder.WriteBuffer(passData.cameraBuffer);
    },
    [this](const CommandBuffer* cmd, const WorldRenderingData& passData)
    {
        cmd->SetBufferData(passData.cameraBuffer, &mCameraData, sizeof(CameraUniforms));
    });
    
    graph.AddRenderPass<WorldRenderingData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
    {
        const auto& sceneData = blackboard.Get<SceneRenderingData>();
        const auto& backbufferDescriptor = graph.GetDescriptor(sceneData.backbuffer);
        
        RenderTextureDescriptor textureDesc;
        textureDesc.size = backbufferDescriptor.size;
        textureDesc.sampleCount = sceneData.config.sampleCount;
        textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
        textureDesc.clearBuffer = true;
        passData.colorTarget = builder.CreateTexture(textureDesc);
        
        textureDesc.format = TextureFormat::D16_UNorm;
        passData.depthTarget = builder.CreateTexture(textureDesc);
        
        passData.colorTarget = builder.UseColorBuffer(passData.colorTarget);
        passData.depthTarget = builder.UseDepthBuffer(passData.depthTarget);
        passData.cameraBuffer = builder.ReadBuffer(updatePass.cameraBuffer);
        blackboard.Add(passData);
    },
    [this](const CommandBuffer* cmd, const WorldRenderingData& passData)
    {
        for (auto& [material, meshList] : mOpaqueQueue)
        {
            for (const auto& pass : material->GetPasses())
            {
                cmd->BindGraphicsPipeline(pass.pipelineState, pass.vertexFunction, pass.fragmentFunction);
                cmd->BindBuffer(passData.cameraBuffer, 0, 0, ShaderStage_Vertex);
                
                for (const auto& element : meshList)
                {
                    const auto& meshBuffer = element.mesh->GetBuffer();
                    cmd->BindBuffer(meshBuffer.GetPositionBuffer(), 0, 0, ShaderStage_Vertex);
                    cmd->BindBuffer(meshBuffer.GetInterleavedBuffer(), 0, 1, ShaderStage_Vertex);
                    
                    ForwardPassUniforms uniforms;
                    uniforms.modelMatrix = element.transform;
                    cmd->SetPushConstant(uniforms, ShaderStage_Vertex);
					
                    for (const auto& descriptor : element.mesh->GetSubmeshDescriptors())
                    {
                        cmd->DrawIndexed(meshBuffer.GetIndexBuffer(), IndexType::UINT32, descriptor.indexCount, 1, descriptor.firstIndex, descriptor.baseVertex, 0);
                    }
                }
            }
        }
        mOpaqueQueue.clear();
    });
}

void WorldRenderer::DrawMesh(const MeshRenderer& meshRenderer, const Transform& transform)
{
    GLEAM_ASSERT(meshRenderer.GetMesh()->GetSubmeshCount() > 0);
    
    const auto& material = meshRenderer.GetMaterial(0);
    const auto& baseMaterial = std::static_pointer_cast<Material>(material->GetBaseMaterial());
    if (baseMaterial->GetRenderQueue() == RenderQueue::Opaque)
    {
        mOpaqueQueue[baseMaterial].push_back({ meshRenderer.GetMesh().get(), meshRenderer.GetMaterial(0).get(), transform.GetTransform() });
    }
    else
    {
        mTransparentQueue[baseMaterial].push_back({ meshRenderer.GetMesh().get(), meshRenderer.GetMaterial(0).get(), transform.GetTransform() });
    }
}

void WorldRenderer::UpdateCamera(const Camera& camera)
{
    mCameraData.viewMatrix = camera.GetViewMatrix();
    mCameraData.projectionMatrix = camera.GetProjectionMatrix();
    mCameraData.viewProjectionMatrix = mCameraData.projectionMatrix * mCameraData.viewMatrix;
	mCameraData.invViewMatrix = Math::Inverse(mCameraData.viewMatrix);
	mCameraData.invProjectionMatrix = Math::Inverse(mCameraData.projectionMatrix);
	mCameraData.invViewProjectionMatrix = Math::Inverse(mCameraData.viewProjectionMatrix);
	mCameraData.worldPosition = camera.GetWorldPosition();
}
