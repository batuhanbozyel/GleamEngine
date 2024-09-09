#include "gpch.h"
#include "MaterialSystem.h"

using namespace Gleam;

void MaterialSystem::Initialize()
{
	
}

void MaterialSystem::Shutdown()
{

}

Material* MaterialSystem::GetMaterial(const AssetReference& ref) const
{
	
	auto it = mMaterials.find(ref);
	if (it == mMaterials.end())
	{
		GLEAM_CORE_ERROR("Material could not found for GUID: {0}", ref.guid.ToString());
		GLEAM_ASSERT(false);
		return nullptr;
	}
	return it->second;
}
