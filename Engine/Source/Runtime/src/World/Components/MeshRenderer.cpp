#include "gpch.h"
#include "MeshRenderer.h"

#include "Core/Application.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material/MaterialSystem.h"

using namespace Gleam;

MeshRenderer::MeshRenderer(const RefCounted<Mesh>& mesh)
	: mMesh(mesh)
{
	PipelineStateDescriptor pipelineDesc;
	pipelineDesc.cullingMode = CullMode::Back;
	pipelineDesc.depthState.writeEnabled = true;

	MaterialDescriptor materialDesc;
	materialDesc.passes.push_back({ .pipelineState = pipelineDesc,
									.vertexEntry = "forwardPassVertexShader",
									.fragmentEntry = "forwardPassFragmentShader" });
	materialDesc.renderQueue = RenderQueue::Opaque;

	static auto materialSystem = GameInstance->GetSubsystem<MaterialSystem>();
	const auto& baseMaterial = materialSystem->CreateMaterial(materialDesc);

	mMaterials.reserve(mesh->GetSubmeshCount());
	for (uint32_t i = 0; i < mMaterials.size(); i++)
	{
		mMaterials[i] = baseMaterial->CreateInstance();
	}
}

void MeshRenderer::SetMaterial(const RefCounted<MaterialInstance>& material, uint32_t index)
{
	GLEAM_ASSERT(mMaterials.size() > index, "Material index out of range.");
	mMaterials[index] = material;
}

const RefCounted<MaterialInstance>& MeshRenderer::GetMaterial(uint32_t index) const
{
	GLEAM_ASSERT(mMaterials.size() > index, "Material index out of range.");
	return mMaterials[index];
}

const RefCounted<Mesh>& MeshRenderer::GetMesh() const
{
	return mMesh;
}