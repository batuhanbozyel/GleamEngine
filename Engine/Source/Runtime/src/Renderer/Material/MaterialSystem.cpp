#include "gpch.h"
#include "MaterialSystem.h"

using namespace Gleam;

RefCounted<Material> MaterialSystem::GetDefaultMaterial()
{
	if (mDefaultMaterial == nullptr)
	{
		PipelineStateDescriptor pipelineDesc;
		pipelineDesc.cullingMode = CullMode::Back;
		pipelineDesc.depthState.writeEnabled = true;
		pipelineDesc.depthState.compareFunction = CompareFunction::Less;

		MaterialDescriptor materialDesc;
		materialDesc.passes.push_back({ .pipelineState = pipelineDesc,
										.vertexEntry = "forwardPassVertexShader",
										.fragmentEntry = "forwardPassFragmentShader" });
		materialDesc.properties = {
			MaterialProperty{ "BaseColor", MaterialPropertyType::Vector4 },
			MaterialProperty{ "Metallic", MaterialPropertyType::Scalar },
			MaterialProperty{ "Roughness", MaterialPropertyType::Scalar },
			MaterialProperty{ "Emissive", MaterialPropertyType::Scalar },
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
