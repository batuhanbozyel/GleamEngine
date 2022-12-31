#include "gpch.h"
#include "ForwardPass.h"

using namespace Gleam;

void ForwardPass::Execute(const CommandBuffer& cmd, const RenderingData& data)
{
	cmd.SetVertexBuffer(data.cameraBuffer, RendererBindingTable::CameraBuffer);
    for (const auto& element : mMeshQueue)
    {
        const auto& meshBuffer = element.mesh->GetBuffer();
		cmd.BindPipeline(PipelineStateDescriptor(), element.program);
        cmd.SetVertexBuffer(meshBuffer.GetPositionBuffer(), RendererBindingTable::Buffer0);
        cmd.SetVertexBuffer(meshBuffer.GetInterleavedBuffer(), RendererBindingTable::Buffer1);
        
        ForwardPassUniforms uniforms;
        uniforms.color = Color::HSVToRGB(static_cast<float>(Time::time), 1.0f, 1.0f);
        cmd.SetPushConstant(uniforms, ShaderStage_Vertex | ShaderStage_Fragment);
        
        for (const auto& submesh : element.mesh->GetSubmeshDescriptors())
        {
            cmd.DrawIndexed(meshBuffer.GetIndexBuffer(), submesh.indexCount, 1, submesh.firstIndex, submesh.baseVertex);
        }
    }
}

void ForwardPass::Finish()
{
    mMeshQueue.clear();
}

void ForwardPass::DrawMesh(const Mesh* mesh, const Material& material, const Matrix4& transform)
{
    mMeshQueue.push_back({mesh, material, transform});
}
