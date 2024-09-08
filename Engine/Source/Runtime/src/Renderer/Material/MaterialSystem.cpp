#include "gpch.h"
#include "MaterialSystem.h"

using namespace Gleam;

void MaterialSystem::Initialize()
{
	
}

void MaterialSystem::Shutdown()
{

}

Material* MaterialSystem::Get(const TString& name) const
{
	auto it = std::find_if(mMaterials.begin(), mMaterials.end(), [&](const auto& material)
	{
		return material->GetName() == name;
	});

	if (it == mMaterials.end())
	{
		GLEAM_CORE_ERROR("Material could not found: {0}", name);
		GLEAM_ASSERT(false);
		return nullptr;
	}
	return it->get();
}
