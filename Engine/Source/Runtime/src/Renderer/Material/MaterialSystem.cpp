#include "gpch.h"
#include "MaterialSystem.h"

using namespace Gleam;

RefCounted<Material> MaterialSystem::CreateMaterial(const MaterialDescriptor& descriptor)
{
    auto it = mMaterials.find(descriptor);
    if (it != mMaterials.end())
    {
        return it->second;
    }

	auto material = CreateRef<Material>(descriptor);
    it = mMaterials.insert(mMaterials.end(), { descriptor, material });
    return material;
}
