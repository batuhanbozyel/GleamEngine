#include "gpch.h"
#include "MaterialSystem.h"

using namespace Gleam;

void MaterialSystem::Initialize()
{

}

void MaterialSystem::Shutdown()
{

}

RefCounted<Material> MaterialSystem::GetDefaultMaterial()
{
	if (mDefaultMaterial == nullptr)
	{
		MaterialDescriptor materialDesc;
		materialDesc.depthState.writeEnabled = true;
		materialDesc.depthState.compareFunction = CompareFunction::Less;
		materialDesc.cullingMode = CullMode::Back;
		materialDesc.properties = {
			MaterialProperty{ "BaseColor", MaterialPropertyType::Float4 },
			MaterialProperty{ "Metallic", MaterialPropertyType::Scalar },
			MaterialProperty{ "Roughness", MaterialPropertyType::Scalar },
			MaterialProperty{ "Emission", MaterialPropertyType::Scalar },
			MaterialProperty{ "BaseColorTexture", MaterialPropertyType::Texture2D },
			MaterialProperty{ "NormalTexture", MaterialPropertyType::Texture2D },
			MaterialProperty{ "MetallicRoughnessTexture", MaterialPropertyType::Texture2D },
			MaterialProperty{ "EmissiveTexture", MaterialPropertyType::Texture2D },
			MaterialProperty{ "OcclusionTexture", MaterialPropertyType::Texture2D }
		};
		materialDesc.renderQueue = RenderQueue::Opaque;
		mDefaultMaterial = CreateMaterial(materialDesc);
	}
	return mDefaultMaterial;
}

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