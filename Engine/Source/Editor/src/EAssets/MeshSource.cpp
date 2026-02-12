#include "MeshSource.h"
#include "TextureSource.h"
#include "MaterialSource.h"
#include "EAssetManager.h"

#include "Bakers/MeshBaker.h"
#include "Bakers/PrefabBaker.h"
#include "Bakers/TextureBaker.h"
#include "Bakers/MaterialBaker.h"

#include "Tools/MeshTools.h"

#include "World/World.h"
#include "Core/Globals.h"
#include "Assets/AssetManager.h"
#include "World/Components/MeshRenderer.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace GEditor;

static RawMesh ProcessAttributes(const cgltf_primitive& primitive, const MeshSource::ImportSettings& settings);
static RawMaterial ProcessMaterial(const cgltf_material& material, const MeshSource::ImportSettings& settings);
static Gleam::TString GetNodeName(const cgltf_node& node, const Gleam::TString& fallback);

bool MeshSource::Import(const Gleam::Path& path, const ImportSettings& settings)
{
	Gleam::TString gltfPath = path.String();
	cgltf_options options = {};
	cgltf_data* data = nullptr;

	cgltf_result result = cgltf_parse_file(&options, gltfPath.c_str(), &data);
	if (result != cgltf_result_success)
	{
		return false;
	}

	result = cgltf_load_buffers(&options, data, gltfPath.c_str());
	if (result != cgltf_result_success)
	{
        cgltf_free(data);
		return false;
	}

    Gleam::TString filename = path.Stem().String();
    Gleam::TArray<RawMaterial> rawMaterials;
	Gleam::HashMap<const cgltf_mesh*, Gleam::RefCounted<MeshBaker>> meshes;
	for (uint32_t nodeIdx = 0; nodeIdx < data->nodes_count; ++nodeIdx)
	{
		const auto& node = data->nodes[nodeIdx];
		auto mesh = node.mesh;
		if (mesh == nullptr)
		{
			continue;
		}
		
		// mesh already imported
		auto meshIt = meshes.find(mesh);
		if (meshIt != meshes.end())
		{
			continue;
		}
		
		Gleam::TArray<RawMesh> rawMeshes;
		for(uint32_t meshIdx = 0; meshIdx < mesh->primitives_count; ++meshIdx)
		{
			auto rawMesh = ProcessAttributes(mesh->primitives[meshIdx], settings);
			
			RawMaterial rawMaterial;
			if (auto mat = mesh->primitives[meshIdx].material; mat != nullptr)
			{
				rawMaterial = ProcessMaterial(*mat, settings);
				if (mat->name)
				{
					rawMaterial.name = mat->name;
				}
				else
				{
					if (mesh->name)
					{
						rawMaterial.name = mesh->name;
					}
					else
					{
						Gleam::TStringStream ss;
						ss << filename << nodeIdx * data->nodes_count + meshIdx;
						rawMaterial.name = ss.str();
					}
				}
			}
			
			// insert unique material
			auto materialIt = std::find(rawMaterials.begin(), rawMaterials.end(), rawMaterial);
			if (materialIt == rawMaterials.end())
			{
				uint32_t materialIdx = static_cast<uint32_t>(rawMaterials.size());
				rawMaterials.push_back(rawMaterial);
				rawMesh.material = materialIdx;
			}
			else
			{
				uint32_t materialIdx = static_cast<uint32_t>(std::distance(rawMaterials.begin(), materialIt));
				rawMesh.material = materialIdx;
			}
			rawMeshes.push_back(rawMesh);
		}
		
		Gleam::MeshDescriptor descriptor = MeshTools::CombineMeshes(rawMeshes);
		if (mesh->name)
		{
			descriptor.name = mesh->name;
		}
		else
		{
			Gleam::TStringStream ss;
			ss << filename << nodeIdx;
			descriptor.name = ss.str();
		}
		meshes[mesh] = EmplaceBaker<MeshBaker>(descriptor);
	}
	
	Gleam::TArray<Gleam::RefCounted<MaterialInstanceBaker>> materials = ImportMaterials(rawMaterials, path, settings);

	// Create prefab
	if (data->nodes_count > 0)
	{
		auto world = Gleam::CreateRef<Gleam::World>(filename);
		auto ProcessNode = [&](auto self, const cgltf_node& node, const Gleam::TString& name) -> Gleam::EntityHandle
		{
			auto& entity = world->GetEntityManager().CreateEntity(name, Gleam::Guid::NewGuid());
			if (node.has_translation)
			{
				entity.SetTranslation(Gleam::Float3(node.translation[0], node.translation[1], node.translation[2]));
			}

			if (node.has_rotation)
			{
				entity.SetRotation(Gleam::Quaternion(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]));
			}

			if (node.has_scale)
			{
				GLEAM_WARN("Mesh source has non-uniform scaling. Using average for scale");
				entity.SetScale((node.scale[0] + node.scale[1] + node.scale[2]) / 3.0f);
			}

			if (node.has_matrix)
			{
				Gleam::Transform transform;

				// TODO: check for non-uniform scaling, then bake if there is
				Gleam::Math::Decompose(Gleam::Float4x4((float*)node.matrix), transform.position, transform.rotation, transform.scale);
				entity.SetLocalTransform(transform);
			}

			if (node.mesh)
			{
				const auto& meshBaker = meshes[node.mesh];
				const auto& meshDesc = meshBaker->GetDescriptor();
				const auto& meshItem = Registry()->GetAsset<Gleam::MeshDescriptor>(meshBaker->Filename());

				Gleam::TArray<Gleam::AssetReference> materialRefs;
				materialRefs.reserve(materials.size());
				for (const auto& materialBaker : materials)
				{
					const auto& materialItem = Registry()->GetAsset<Gleam::MaterialInstanceDescriptor>(materialBaker->Filename());
					materialRefs.push_back(materialItem.reference);
				}
				entity.AddComponent<Gleam::MeshRenderer>(meshItem.reference, materialRefs);
			}

			if (node.children_count > 0)
			{
				for (uint32_t childIdx = 0; childIdx < node.children_count; ++childIdx)
				{
					const auto& childNode = node.children[childIdx];
					auto childName = GetNodeName(*childNode, filename + std::to_string(childIdx));
					auto childHandle = self(self, *childNode, childName);
					auto& childEntity = world->GetEntityManager().GetComponent<Gleam::Entity>(childHandle);
					childEntity.SetParent(entity);
				}
			}
			return entity;
		};
		
		Gleam::HashSet<const cgltf_node*> rootNodes;
		for (uint32_t nodeIdx = 0; nodeIdx < data->nodes_count; ++nodeIdx)
		{
			const auto& node = data->nodes[nodeIdx];
			if (node.parent == nullptr)
			{
				rootNodes.emplace(&node);
			}
		}

		if (rootNodes.size() == 1)
		{
			const auto root = *rootNodes.begin();
			auto entityName = GetNodeName(*root, filename);
			auto entity = ProcessNode(ProcessNode , *root, entityName);
		}
		else
		{
			auto& rootEntity = world->GetEntityManager().CreateEntity(filename, Gleam::Guid::NewGuid());

			uint32_t nodeIdx = 1;
			for (auto node : rootNodes)
			{
				auto entityName = GetNodeName(*node, filename + std::to_string(nodeIdx));
				auto entityHandle = ProcessNode(ProcessNode, *node, entityName);
				auto& entity = world->GetEntityManager().GetComponent<Gleam::Entity>(entityHandle);
				entity.SetParent(rootEntity);
				nodeIdx++;
			}
		}
		EmplaceBaker<PrefabBaker>(world);
	}

	cgltf_free(data);
    return true;
}

Gleam::TArray<Gleam::RefCounted<MaterialInstanceBaker>> MeshSource::ImportMaterials(const Gleam::TArray<RawMaterial>& rawMaterials, const Gleam::Path& path, const ImportSettings& settings)
{
	auto directory = path.Parent();
	auto opaqueLitMaterialAsset = AssetManager()->GetAsset<Gleam::MaterialDescriptor>("Materials/OpaqueLit").reference;
	auto transparentLitMaterialAsset = AssetManager()->GetAsset<Gleam::MaterialDescriptor>("Materials/TransparentLit").reference;

	auto assetManager = Gleam::Globals::GameInstance->GetSubsystem<Gleam::AssetManager>();
	auto opaqueLitMaterial = assetManager->LoadDescriptor<Gleam::MaterialDescriptor>(opaqueLitMaterialAsset);
	auto transparentLitMaterial = assetManager->LoadDescriptor<Gleam::MaterialDescriptor>(transparentLitMaterialAsset);

	Gleam::TArray<Gleam::RefCounted<MaterialInstanceBaker>> materials;
	materials.reserve(rawMaterials.size());
	for (const auto& material : rawMaterials)
	{
		Gleam::MaterialInstanceDescriptor descriptor;
		descriptor.name = material.name;

		if (material.alphaBlend)
		{
			descriptor.material = transparentLitMaterialAsset;
			descriptor.properties = transparentLitMaterial.properties;
		}
		else
		{
			descriptor.material = opaqueLitMaterialAsset;
			descriptor.properties = opaqueLitMaterial.properties;
		}

		descriptor["BaseColor"] = material.albedoColor;
		descriptor["Emission"] = material.emissiveColor;
		descriptor["Metallic"] = material.metallicFactor;
		descriptor["Roughness"] = material.roughnessFactor;

		if (const auto& texture = material.textures[PBRTexture::Albedo]; texture.Empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSettings = TextureSource::ImportSettings();
			textureSettings.colorSpace = TextureColorSpace::sRGB;
			textureSettings.generateMips = true;
			if (ImportReference<TextureSource>(texturePath, textureSettings))
			{
				descriptor["BaseColorTexture"] = Registry()->GetAsset<Gleam::Texture2DDescriptor>(texture.Stem()).reference;
			}
		}

		if (const auto& texture = material.textures[PBRTexture::Normal]; texture.Empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSettings = TextureSource::ImportSettings();
			textureSettings.generateMips = true;
			if (ImportReference<TextureSource>(texturePath, textureSettings))
			{
				descriptor["NormalTexture"] = Registry()->GetAsset<Gleam::Texture2DDescriptor>(texture.Stem()).reference;
			}
		}

		if (const auto& texture = material.textures[PBRTexture::MetallicRoughness]; texture.Empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSettings = TextureSource::ImportSettings();
			textureSettings.generateMips = true;
			if (ImportReference<TextureSource>(texturePath, textureSettings))
			{
				descriptor["MetallicRoughnessTexture"] = Registry()->GetAsset<Gleam::Texture2DDescriptor>(texture.Stem()).reference;
			}
		}

		if (const auto& texture = material.textures[PBRTexture::Emissive]; texture.Empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSettings = TextureSource::ImportSettings();
			textureSettings.colorSpace = TextureColorSpace::sRGB;
			textureSettings.generateMips = true;
			if (ImportReference<TextureSource>(texturePath, textureSettings))
			{
				descriptor["EmissiveTexture"] = Registry()->GetAsset<Gleam::Texture2DDescriptor>(texture.Stem()).reference;
			}
		}
		materials.emplace_back(EmplaceBaker<MaterialInstanceBaker>(descriptor));
	}
	return materials;
}

RawMesh ProcessAttributes(const cgltf_primitive& primitive, const MeshSource::ImportSettings& settings)
{
    RawMesh mesh;
    uint32_t vertexCount = static_cast<uint32_t>(primitive.attributes[0].data->count);
    
    // process indices
    if (primitive.indices)
    {
        mesh.indices.resize(primitive.indices->count);
        cgltf_accessor_unpack_indices(primitive.indices, mesh.indices.data(), mesh.indices.size());
    }
    else
    {
        mesh.indices.resize(vertexCount);
        for (uint32_t idx = 0; idx < vertexCount; ++idx)
        {
            mesh.indices[idx] = idx;
        }
    }

    // process attributes
    for (uint32_t ai = 0; ai < primitive.attributes_count; ++ai)
    {
        const auto& attribute = primitive.attributes[ai];
        if (attribute.type == cgltf_attribute_type_position)
        {
            mesh.positions.resize(vertexCount);
            cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)mesh.positions.data(), mesh.positions.size() * 3);
        }
        else if (attribute.type == cgltf_attribute_type_normal)
        {
            mesh.normals.resize(vertexCount);
            cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)mesh.normals.data(), mesh.normals.size() * 3);
        }
		else if (attribute.type == cgltf_attribute_type_tangent)
		{
			mesh.tangents.resize(vertexCount);
			cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)mesh.tangents.data(), mesh.tangents.size() * 4);
		}
        else if (attribute.type == cgltf_attribute_type_texcoord)
        {
            mesh.texCoords.resize(vertexCount);
            cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)mesh.texCoords.data(), mesh.texCoords.size() * 2);
        }
    }
	
	if (mesh.normals.empty())
	{
		MeshTools::ComputeSmoothNormals(mesh);
	}

	MeshTools::RemoveDegenerateFaces(mesh);
	if (mesh.tangents.empty())
	{
		if (mesh.texCoords.empty())
		{
			mesh.tangents.resize(mesh.normals.size(), Gleam::Float4(1.0f, 0.0f, 0.0f, 1.0f));
		}
		else
		{
			MeshTools::ComputeTangents(mesh);
		}
	}

	if (mesh.texCoords.empty())
	{
		mesh.texCoords.resize(mesh.positions.size(), Gleam::Float2::zero);
	}

    return mesh;
}

RawMaterial ProcessMaterial(const cgltf_material& mat, const MeshSource::ImportSettings& settings)
{
	RawMaterial material;
    
    // Albedo - Metallic - Roughness
    if (mat.has_pbr_metallic_roughness)
    {
        const auto& pbr = mat.pbr_metallic_roughness;
        material.albedoColor = Gleam::Color(pbr.base_color_factor[0],
                                            pbr.base_color_factor[1],
                                            pbr.base_color_factor[2],
                                            pbr.base_color_factor[3]);
        
        material.metallicFactor = pbr.metallic_factor;
        material.roughnessFactor = pbr.roughness_factor;
        
        if (auto texture = pbr.base_color_texture.texture; texture != nullptr)
        {
			Gleam::Path file = texture->image->uri;
            material.textures[PBRTexture::Albedo] = file;
        }
        
        if (auto texture = pbr.metallic_roughness_texture.texture; texture != nullptr)
        {
			Gleam::Path file = texture->image->uri;
            material.textures[PBRTexture::MetallicRoughness] = file;
        }
    }
    
    // Normal
    if (auto texture = mat.normal_texture.texture; texture != nullptr)
    {
		Gleam::Path file = texture->image->uri;
        material.textures[PBRTexture::Normal] = file;
    }
    
    // Emissive
    if (auto texture = mat.emissive_texture.texture; texture != nullptr)
    {
		Gleam::Path file = texture->image->uri;
        material.textures[PBRTexture::Emissive] = file;
    }
    
    float emissiveStrength = mat.has_emissive_strength ? mat.emissive_strength.emissive_strength : 1.0f;
    material.emissiveColor = Gleam::Color(mat.emissive_factor[0] * emissiveStrength,
                                          mat.emissive_factor[1] * emissiveStrength,
                                          mat.emissive_factor[2] * emissiveStrength,
                                          1.0f);
    
    material.doubleSided = mat.double_sided;
    material.alphaCutoff = mat.alpha_cutoff;
    material.unlit = mat.unlit;
    
    switch (mat.alpha_mode)
    {
        case cgltf_alpha_mode_opaque:
        {
            material.alphaBlend = false;
            break;
        }
        case cgltf_alpha_mode_blend:
		case cgltf_alpha_mode_mask:
        {
            material.alphaBlend = true;
            break;
        }
        default:
        {
            GLEAM_ASSERT(false, "glTF file is corrupted: invalid alpha_mode");
            break;
        }
    }
    
    return material;
}

Gleam::TString GetNodeName(const cgltf_node& node, const Gleam::TString& fallback)
{
	if (node.name)
	{
		return Gleam::TString(node.name);
	}

	if (node.mesh && node.mesh->name)
	{
		return Gleam::TString(node.mesh->name);
	}

	return fallback;
}