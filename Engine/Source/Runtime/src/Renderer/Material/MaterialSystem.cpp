#include "gpch.h"
#include "MaterialSystem.h"

using namespace Gleam;

void MaterialSystem::Initialize()
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
	mDefaultMaterial = CreateRef<Material>(materialDesc);
}

void MaterialSystem::Shutdown()
{

}

RefCounted<Material> MaterialSystem::GetDefaultMaterial() const
{
	return mDefaultMaterial;
}
