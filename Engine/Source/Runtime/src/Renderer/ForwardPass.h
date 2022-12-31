#pragma once
#include "Mesh.h"
#include "RenderPass.h"

namespace Gleam {

class ForwardPass : public RenderPass
{
public:

	virtual RenderPassDescriptor Configure(RendererContext& context) override;

    virtual void Execute(const CommandBuffer& cmd, const RenderingData& data) override;
    
    virtual void Finish() override;

    void DrawMesh(const Mesh* mesh, const Material& material, const Matrix4& transform);

private:

    TArray<MeshQueueElement> mMeshQueue;

};

} // namespace Gleam
