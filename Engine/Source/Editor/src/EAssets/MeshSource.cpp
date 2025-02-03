#include "Gleam.h"
#include "MeshSource.h"
#include "TextureSource.h"
#include "MaterialSource.h"
#include "EAssetManager.h"

#include "Bakers/MeshBaker.h"
#include "Bakers/PrefabBaker.h"
#include "Bakers/TextureBaker.h"
#include "Bakers/MaterialBaker.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace GEditor;

static RawMesh ProcessAttributes(const cgltf_primitive& primitive, const MeshSource::ImportSettings& settings);
static RawMaterial ProcessMaterial(const cgltf_material& material, const MeshSource::ImportSettings& settings);

static Gleam::MeshDescriptor CombineMeshes(const Gleam::TArray<RawMesh>& meshes);
static Gleam::TArray<Gleam::InterleavedMeshVertex> InterleaveMeshVertices(const RawMesh& mesh);
static Gleam::BoundingBox CalculateBounds(const Gleam::TArray<Gleam::Float3>& positions);

bool MeshSource::Import(const Gleam::Filesystem::Path& path, const ImportSettings& settings)
{
	Gleam::TString gltfPath = path.string();
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

	auto directory = path.parent_path();
    auto filename = path.stem().string();
	
    Gleam::TArray<RawMaterial> rawMaterials;
	Gleam::HashMap<const cgltf_mesh*, Gleam::RefCounted<MeshBaker>> meshes;
	for (uint32_t i = 0; i < data->meshes_count; ++i)
	{
		const auto& mesh = data->meshes[i];
		
		Gleam::TArray<RawMesh> rawMeshes;
		rawMeshes.reserve(mesh.primitives_count);
		for(uint32_t meshIdx = 0; meshIdx < mesh.primitives_count; ++meshIdx)
		{
			auto rawMesh = ProcessAttributes(mesh.primitives[meshIdx], settings);
			
			RawMaterial rawMaterial;
			if (auto mat = mesh.primitives[meshIdx].material; mat != nullptr)
			{
				rawMaterial = ProcessMaterial(*mat, settings);
				if (mat->name)
				{
					rawMaterial.name = mat->name;
				}
				else
				{
					if (mesh.name)
					{
						rawMaterial.name = mesh.name;
					}
					else
					{
						Gleam::TStringStream ss;
						ss << filename << i * data->meshes_count + meshIdx;
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
		
		Gleam::MeshDescriptor descriptor = CombineMeshes(rawMeshes);
		if (mesh.name)
		{
			descriptor.name = mesh.name;
		}
		else
		{
			Gleam::TStringStream ss;
			ss << filename << i;
			descriptor.name = ss.str();
		}
		meshes[&mesh] = EmplaceBaker<MeshBaker>(descriptor);
	}
	
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

		if (const auto& texture = material.textures[PBRTexture::Albedo]; texture.empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSettings = TextureSource::ImportSettings();
			if (ImportReference<TextureSource>(texturePath, textureSettings))
			{
				descriptor["BaseColorTexture"] = Registry()->GetAsset<Gleam::Texture2DDescriptor>(texture).reference;
			}
		}

		if (const auto& texture = material.textures[PBRTexture::Normal]; texture.empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSettings = TextureSource::ImportSettings();
			if (ImportReference<TextureSource>(texturePath, textureSettings))
			{
				descriptor["NormalTexture"] = Registry()->GetAsset<Gleam::Texture2DDescriptor>(texture).reference;
			}
		}

		if (const auto& texture = material.textures[PBRTexture::MetallicRoughness]; texture.empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSettings = TextureSource::ImportSettings();
			if (ImportReference<TextureSource>(texturePath, textureSettings))
			{
				descriptor["MetallicRoughnessTexture"] = Registry()->GetAsset<Gleam::Texture2DDescriptor>(texture).reference;
			}
		}

		if (const auto& texture = material.textures[PBRTexture::Emissive]; texture.empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSettings = TextureSource::ImportSettings();
			if (ImportReference<TextureSource>(texturePath, textureSettings))
			{
				descriptor["EmissiveTexture"] = Registry()->GetAsset<Gleam::Texture2DDescriptor>(texture).reference;
			}
		}
		materials.emplace_back(EmplaceBaker<MaterialInstanceBaker>(descriptor));
    }

	// Create prefab
	if (data->nodes_count > 0)
	{
		auto world = Gleam::CreateRef<Gleam::World>(filename);
		for (uint32_t i = 0; i < data->nodes_count; ++i)
		{
			const auto& node = data->nodes[i];
			if (node.mesh)
			{
				const auto& meshBaker = meshes[node.mesh];
				const auto& meshDesc = meshBaker->GetDescriptor();
				const auto& meshItem = Registry()->GetAsset<Gleam::MeshDescriptor>(meshBaker->Filename());

				Gleam::TArray<Gleam::AssetReference> materialRefs;
				materialRefs.reserve(meshDesc.submeshes.size());
				for (const auto& submesh : meshDesc.submeshes)
				{
					const auto& materialBaker = materials[submesh.materialIndex];
					const auto& materialItem = Registry()->GetAsset<Gleam::MaterialInstanceDescriptor>(materialBaker->Filename());
					materialRefs.push_back(materialItem.reference);
				}

				auto& entity = world->GetEntityManager().CreateEntity(meshDesc.name, Gleam::Guid::NewGuid());
				entity.AddComponent<Gleam::MeshRenderer>(meshItem.reference, materialRefs);

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
					entity.SetScale(Gleam::Float3(node.scale[0], node.scale[1], node.scale[2]));
				}
			}
		}
		EmplaceBaker<PrefabBaker>(world);
	}

	cgltf_free(data);
    return true;
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
        else if (attribute.type == cgltf_attribute_type_texcoord)
        {
            mesh.texCoords.resize(vertexCount);
            cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)mesh.texCoords.data(), mesh.texCoords.size() * 2);
        }
    }
    
    // TODO: calculate normals
    if (mesh.normals.empty())
    {
        mesh.normals.resize(vertexCount, Gleam::Float3(0.5f, 0.5f, 1.0f));
    }
    
    if (mesh.texCoords.empty())
    {
        mesh.texCoords.resize(vertexCount, Gleam::Float2::zero);
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
			Gleam::Filesystem::Path file = texture->image->uri;
            material.textures[PBRTexture::Albedo] = file;
        }
        
        if (auto texture = pbr.metallic_roughness_texture.texture; texture != nullptr)
        {
			Gleam::Filesystem::Path file = texture->image->uri;
            material.textures[PBRTexture::MetallicRoughness] = file;
        }
    }
    
    // Normal
    if (auto texture = mat.normal_texture.texture; texture != nullptr)
    {
		Gleam::Filesystem::Path file = texture->image->uri;
        material.textures[PBRTexture::Normal] = file;
    }
    
    // Emissive
    if (auto texture = mat.emissive_texture.texture; texture != nullptr)
    {
		Gleam::Filesystem::Path file = texture->image->uri;
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

Gleam::MeshDescriptor CombineMeshes(const Gleam::TArray<RawMesh>& meshes)
{
    Gleam::MeshDescriptor combined;
    combined.submeshes.resize(meshes.size());
    
    Gleam::SubmeshDescriptor submesh;
    for (uint32_t i = 0; i < meshes.size(); ++i)
    {
        const auto& mesh = meshes[i];
		submesh.materialIndex = mesh.material;
        submesh.bounds = CalculateBounds(mesh.positions);
        submesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
        combined.submeshes[i] = submesh;
        
        auto interleaved = InterleaveMeshVertices(mesh);
        combined.indices.insert(combined.indices.end(), mesh.indices.begin(), mesh.indices.end());
        combined.positions.insert(combined.positions.end(), mesh.positions.begin(), mesh.positions.end());
        combined.interleavedVertices.insert(combined.interleavedVertices.end(), interleaved.begin(), interleaved.end());
        
        submesh.baseVertex += static_cast<uint32_t>(mesh.positions.size());
        submesh.firstIndex += static_cast<uint32_t>(mesh.indices.size());
    }
	return combined;
}

Gleam::TArray<Gleam::InterleavedMeshVertex> InterleaveMeshVertices(const RawMesh& mesh)
{
	Gleam::TArray<Gleam::InterleavedMeshVertex> interleaved(mesh.normals.size());
	for (uint32_t i = 0; i < mesh.normals.size(); ++i)
	{
		interleaved[i].normal = mesh.normals[i];
		interleaved[i].texCoord = mesh.texCoords[i];
	}
	return interleaved;
}

Gleam::BoundingBox CalculateBounds(const Gleam::TArray<Gleam::Float3>& positions)
{
    Gleam::BoundingBox bounds(Gleam::Math::Infinity, Gleam::Math::NegativeInfinity);
    for (const auto& position : positions)
    {
        bounds.min = Gleam::Math::Min(bounds.min, position);
        bounds.max = Gleam::Math::Max(bounds.max, position);
    }
    return bounds;
}
