#include "gpch.h"
#include "MeshRenderer.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material/MaterialSystem.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetReference.h"

using namespace Gleam;

MeshRenderer::MeshRenderer(const AssetReference& mesh, const TArray<AssetReference>& materials)
	: mMesh(Globals::Engine->GetSubsystem<AssetManager>()->Get<MeshDescriptor>(mesh))
{
	auto materialSystem = Globals::Engine->GetSubsystem<MaterialSystem>();
	auto baseMaterial = materialSystem->GetMaterial(AssetReference());

	mMaterials.reserve(mMesh.GetSubmeshCount());
	for (uint32_t i = 0; i < mMesh.GetSubmeshCount(); i++)
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

const TArray<MaterialInstance>& MeshRenderer::GetMaterials() const
{
	return mMaterials;
}

const Mesh& MeshRenderer::GetMesh() const
{
    return mMesh;
}
