#include "gpch.h"
#include "MeshRenderer.h"

#include "Core/Globals.h"
#include "Core/Application.h"

#include "Renderer/Mesh.h"
#include "Renderer/Material/MaterialSystem.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetReference.h"

using namespace Gleam;

MeshRenderer::MeshRenderer(const AssetReference& mesh, const TArray<AssetReference>& materials)
	: mMesh(Globals::GameInstance->GetSubsystem<AssetManager>()->Get<MeshDescriptor>(mesh))
{
	auto materialSystem = Globals::GameInstance->GetSubsystem<MaterialSystem>();

	mMaterials.reserve(materials.size());
	for (const auto& material : materials)
	{
		auto descriptor = Globals::GameInstance->GetSubsystem<AssetManager>()->Get<MaterialInstanceDescriptor>(material);
		auto baseMaterial = materialSystem->GetMaterial(descriptor.material);
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
