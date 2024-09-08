#include "gpch.h"
#include "MeshRenderer.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material/MaterialSystem.h"

using namespace Gleam;

MeshRenderer::MeshRenderer(const Mesh& mesh)
	: mMesh(mesh)
{
    static auto materialSystem = Globals::Engine->GetSubsystem<MaterialSystem>();
    const auto& baseMaterial = materialSystem->Get("OpaqueLit");

    mMaterials.reserve(mesh.GetSubmeshCount());
    for (uint32_t i = 0; i < mesh.GetSubmeshCount(); i++)
    {
        mMaterials.emplace_back(baseMaterial->CreateInstance());
    }
}

void MeshRenderer::SetMaterial(const MaterialInstance& material, uint32_t index)
{
    GLEAM_ASSERT(mMaterials.size() > index, "Material index out of range.");
    mMaterials[index] = material;
}

const MaterialInstance& MeshRenderer::GetMaterial(uint32_t index) const
{
    GLEAM_ASSERT(mMaterials.size() > index, "Material index out of range.");
    return mMaterials[index];
}

const Mesh& MeshRenderer::GetMesh() const
{
    return mMesh;
}
