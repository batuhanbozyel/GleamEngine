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

static Gleam::TArray<Gleam::InterleavedMeshVertex> InterleaveMeshVertices(const RawMesh& mesh);
static Gleam::MeshDescriptor CombineMeshes(const Gleam::TArray<RawMesh>& meshes);
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

	Gleam::TArray<RawMesh> meshes;
    Gleam::TArray<RawMaterial> materials;
    for(uint32_t i = 0; i < data->meshes_count; ++i)
    {
        const auto& mesh = data->meshes[i];
        for(uint32_t meshIdx = 0; meshIdx < mesh.primitives_count; ++meshIdx)
        {
            auto rawMesh = ProcessAttributes(mesh.primitives[meshIdx], settings);
			if (mesh.name)
			{
				rawMesh.name = mesh.name;
			}
            else
            {
                Gleam::TStringStream ss;
                ss << filename << "_mesh" << i * data->meshes_count + meshIdx;
                rawMesh.name = ss.str();
            }
            
			RawMaterial material;
            if (auto mat = mesh.primitives[meshIdx].material; mat != nullptr)
            {
                material = ProcessMaterial(*mat, settings);
                if (mat->name)
                {
                    material.name = mat->name;
                }
                else
                {
                    Gleam::TStringStream ss;
                    ss << filename << "_material" << i * data->meshes_count + meshIdx;
                    material.name = ss.str();
                }
            }
            
            // insert unique material
            auto materialIt = std::find(materials.begin(), materials.end(), material);
            if (materialIt == materials.end())
            {
                uint32_t materialIdx = static_cast<uint32_t>(materials.size());
                materials.push_back(material);
				rawMesh.material = materialIdx;
            }
            else
            {
				uint32_t materialIdx = static_cast<uint32_t>(std::distance(materials.begin(), materialIt));
                rawMesh.material = materialIdx;
            }
            meshes.push_back(rawMesh);
        }
    }

	PrefabHierarchy hierarchy;
    if (settings.combineMeshes)
    {
        auto combined = CombineMeshes(meshes);
        combined.name = filename;

		auto meshBaker = Gleam::CreateRef<MeshBaker>(combined);
		mBakers.push_back(meshBaker);
		hierarchy.meshes.push_back(meshBaker);
		mRegistry->RegisterAsset<Gleam::MeshDescriptor>(combined.name);
    }
    else
    {
        for (const auto& mesh : meshes)
        {
            Gleam::MeshDescriptor descriptor;
            descriptor.name = mesh.name;
            descriptor.indices = mesh.indices;
            descriptor.positions = mesh.positions;
            descriptor.interleavedVertices = InterleaveMeshVertices(mesh);
            
            Gleam::SubmeshDescriptor submesh;
			submesh.materialIndex = mesh.material;
            submesh.bounds = CalculateBounds(mesh.positions);
            submesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
            descriptor.submeshes.push_back(submesh);

			auto meshBaker = Gleam::CreateRef<MeshBaker>(descriptor);
			mBakers.push_back(meshBaker);
			hierarchy.meshes.push_back(meshBaker);
			mRegistry->RegisterAsset<Gleam::MeshDescriptor>(mesh.name);
        }
    }

	auto opaqueLitMaterialAsset = mAssetManager->GetAsset<Gleam::MaterialDescriptor>("Materials/OpaqueLit").reference;
	auto transparentLitMaterialAsset = mAssetManager->GetAsset<Gleam::MaterialDescriptor>("Materials/TransparentLit").reference;

	auto assetManager = Gleam::Globals::GameInstance->GetSubsystem<Gleam::AssetManager>();
	auto opaqueLitMaterial = assetManager->Get<Gleam::MaterialDescriptor>(opaqueLitMaterialAsset);
	auto transparentLitMaterial = assetManager->Get<Gleam::MaterialDescriptor>(transparentLitMaterialAsset);
    for (const auto& material : materials)
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
			auto textureSource = TextureSource(mAssetManager, mRegistry);
			auto textureSettings = TextureSource::ImportSettings();
			if (textureSource.Import(texturePath, textureSettings))
			{
				descriptor["BaseColorTexture"] = mRegistry->GetAsset<Gleam::TextureDescriptor>(texture).reference;
				mBakers.emplace_back(textureSource.mBakers[0]);
			}
		}

		if (const auto& texture = material.textures[PBRTexture::Normal]; texture.empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSource = TextureSource(mAssetManager, mRegistry);
			auto textureSettings = TextureSource::ImportSettings();
			if (textureSource.Import(texturePath, textureSettings))
			{
				descriptor["NormalTexture"] = mRegistry->GetAsset<Gleam::TextureDescriptor>(texture).reference;
				mBakers.emplace_back(textureSource.mBakers[0]);
			}
		}

		if (const auto& texture = material.textures[PBRTexture::MetallicRoughness]; texture.empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSource = TextureSource(mAssetManager, mRegistry);
			auto textureSettings = TextureSource::ImportSettings();
			if (textureSource.Import(texturePath, textureSettings))
			{
				descriptor["MetallicRoughnessTexture"] = mRegistry->GetAsset<Gleam::TextureDescriptor>(texture).reference;
				mBakers.emplace_back(textureSource.mBakers[0]);
			}
		}

		if (const auto& texture = material.textures[PBRTexture::Emissive]; texture.empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSource = TextureSource(mAssetManager, mRegistry);
			auto textureSettings = TextureSource::ImportSettings();
			if (textureSource.Import(texturePath, textureSettings))
			{
				descriptor["EmissiveTexture"] = mRegistry->GetAsset<Gleam::TextureDescriptor>(texture).reference;
				mBakers.emplace_back(textureSource.mBakers[0]);
			}
		}
		
		auto materialBaker = Gleam::CreateRef<MaterialInstanceBaker>(descriptor);
		mBakers.push_back(materialBaker);
		hierarchy.materials.push_back(materialBaker);
		mRegistry->RegisterAsset<Gleam::MaterialInstanceDescriptor>(material.name);
    }

	// Create prefab
	if (not hierarchy.meshes.empty())
	{
		auto world = Gleam::CreateRef<Gleam::World>(filename);
		for (const auto& meshBaker : hierarchy.meshes)
		{
			const auto& descriptor = meshBaker->GetDescriptor();
			const auto& meshItem = mRegistry->GetAsset<Gleam::MeshDescriptor>(meshBaker->Filename());

			Gleam::TArray<Gleam::AssetReference> materials;
			for (const auto& submesh : descriptor.submeshes)
			{
				const auto& materialBaker = hierarchy.materials[submesh.materialIndex];
				const auto& descriptor = materialBaker->GetDescriptor();
				const auto& materialItem = mRegistry->GetAsset<Gleam::MaterialInstanceDescriptor>(materialBaker->Filename());
				materials.push_back(materialItem.reference);
			}

			auto& entity = world->GetEntityManager().CreateEntity(Gleam::Guid::NewGuid());
			entity.AddComponent<Gleam::MeshRenderer>(meshItem.reference, materials);
		}
		mBakers.emplace_back(Gleam::CreateRef<PrefabBaker>(world));
		mRegistry->RegisterAsset<Gleam::Prefab>(filename);
	}

	cgltf_free(data);
    return true;
}

RawMesh ProcessAttributes(const cgltf_primitive& primitive, const MeshSource::ImportSettings& settings)
{
    RawMesh mesh;
    uint32_t vertex_count = static_cast<uint32_t>(primitive.attributes[0].data->count);
    
    // process indices
    if (primitive.indices)
    {
        mesh.indices.resize(primitive.indices->count);
        cgltf_accessor_unpack_indices(primitive.indices, mesh.indices.data(), mesh.indices.size());
    }
    else
    {
        mesh.indices.resize(vertex_count);
        for (uint32_t idx = 0; idx < vertex_count; ++idx)
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
            mesh.positions.resize(vertex_count);
            cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)mesh.positions.data(), mesh.positions.size() * 3);
        }
        else if (attribute.type == cgltf_attribute_type_normal)
        {
            mesh.normals.resize(vertex_count);
            cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)mesh.normals.data(), mesh.normals.size() * 3);
        }
        else if (attribute.type == cgltf_attribute_type_texcoord)
        {
            mesh.texCoords.resize(vertex_count);
            cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)mesh.texCoords.data(), mesh.texCoords.size() * 2);
        }
    }
    
    // TODO: calculate normals
    if (mesh.normals.empty())
    {
        mesh.normals.resize(vertex_count, Gleam::Float3(0.5f, 0.5f, 1.0f));
    }
    
    if (mesh.texCoords.empty())
    {
        mesh.texCoords.resize(vertex_count, Gleam::Float2::zero);
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
