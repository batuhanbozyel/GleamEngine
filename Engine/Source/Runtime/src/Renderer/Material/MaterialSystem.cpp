#include "gpch.h"
#include "MaterialSystem.h"

using namespace Gleam;

const RefCounted<Material>& MaterialSystem::CreateMaterial(const MaterialDescriptor& descriptor)
{
    auto it = mMaterials.find(descriptor);
    if (it != mMaterials.end())
    {
        return it->second;
    }
    it = mMaterials.insert(mMaterials.end(), {descriptor, CreateRef<Material>(descriptor) });
    return it->second;
}
