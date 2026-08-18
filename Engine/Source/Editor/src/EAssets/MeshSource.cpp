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
#include "Renderer/Shaders/ShaderInterop.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <meshoptimizer.h>

using namespace GEditor;

static RawMesh ProcessAttributes(const cgltf_primitive& primitive, const MeshSource::ImportSettings& settings);
static RawMaterial ProcessMaterial(const cgltf_material& material, const MeshSource::ImportSettings& settings);
static Gleam::TString GetNodeName(const cgltf_node& node, const Gleam::TString& fallback);
static Gleam::Float4x4 GetNodeTransform(const cgltf_node& node);
static bool IsSameTransform(const Gleam::Float4x4& lhs, const Gleam::Float4x4& rhs);
static bool IsNonUniformScale(const Gleam::Float4x4& transform);
static Gleam::Float4x4 ConvertRHtoLH(const Gleam::Float4x4& transform);
static void ConvertRHtoLH(RawMesh& mesh);

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
	Gleam::HashMap<const cgltf_mesh*, Gleam::TArray<RawMesh>> rawMeshes;
	for (uint32_t nodeIdx = 0; nodeIdx < data->nodes_count; ++nodeIdx)
	{
		const auto& node = data->nodes[nodeIdx];
		auto mesh = node.mesh;
		if (mesh == nullptr)
		{
			continue;
		}

		// mesh already processed
		auto it = rawMeshes.find(mesh);
		if (it != rawMeshes.end())
		{
			continue;
		}

		Gleam::TString meshName;
		if (mesh->name)
		{
			meshName = mesh->name;
		}
		else
		{
			Gleam::TStringStream ss;
			ss << filename << nodeIdx;
			meshName = ss.str();
		}

		it = rawMeshes.emplace_hint(rawMeshes.end(), mesh, Gleam::TArray<RawMesh>{});
		Gleam::TArray<RawMesh>& primitives = it->second;
		for(uint32_t meshIdx = 0; meshIdx < mesh->primitives_count; ++meshIdx)
		{
			auto rawMesh = ProcessAttributes(mesh->primitives[meshIdx], settings);
			ConvertRHtoLH(rawMesh);
			rawMesh.name = meshName;
			
			RawMaterial rawMaterial;
			if (auto mat = mesh->primitives[meshIdx].material; mat != nullptr)
			{
				rawMaterial = ProcessMaterial(*mat, settings);
			}
			
			// insert unique material
			auto materialIt = std::find(rawMaterials.begin(), rawMaterials.end(), rawMaterial);
			if (materialIt == rawMaterials.end())
			{
				uint32_t materialIdx = static_cast<uint32_t>(rawMaterials.size());
				if (auto mat = mesh->primitives[meshIdx].material; mat && mat->name)
				{
					rawMaterial.name = mat->name;
				}
				else
				{
					Gleam::TStringStream ss;
					ss << rawMesh.name << "_Material" << materialIdx;
					rawMaterial.name = ss.str();
				}
				rawMaterials.push_back(rawMaterial);
				rawMesh.material = materialIdx;
			}
			else
			{
				uint32_t materialIdx = static_cast<uint32_t>(std::distance(rawMaterials.begin(), materialIt));
				rawMesh.material = materialIdx;
			}
			primitives.push_back(rawMesh);
		}
	}
	
	Gleam::TArray<Gleam::RefCounted<MaterialInstanceBaker>> importedMaterials = ImportMaterials(rawMaterials, path, settings);

	// Create prefab
	if (data->nodes_count > 0)
	{
		struct MeshVariant
		{
			Gleam::RefCounted<MeshBaker> baker;
			Gleam::Float4x4 transform = Gleam::Float4x4::identity;
		};
		Gleam::HashMap<const cgltf_mesh*, Gleam::TArray<MeshVariant>> importedMeshVariants;

		auto GetOrCreateMesh = [&, this](const cgltf_mesh* mesh, const Gleam::Float4x4& transform, bool hierarchyHasNonUniformScaling) -> Gleam::RefCounted<MeshBaker>
		{
			auto& variants = importedMeshVariants[mesh];
			if (hierarchyHasNonUniformScaling)
			{
				for (auto& variant : variants)
				{
					if (IsSameTransform(variant.transform, transform))
					{
						return variant.baker;
					}
				}

				Gleam::TArray<RawMesh> meshes = rawMeshes.at(mesh);
				for (auto& rawMesh : meshes)
				{
					MeshTools::ApplyTransform(rawMesh, transform);
					rawMesh.name += "_Variant" + std::to_string(variants.size());
				}

				MeshVariant newVariant;
				newVariant.baker = ImportMesh(meshes, path, settings);
				newVariant.transform = transform;
				variants.push_back(newVariant);
				return newVariant.baker;
			}

			// Import default variant for uniform scaled meshes
			for (auto& variant : variants)
			{
				if (IsSameTransform(variant.transform, Gleam::Float4x4::identity))
				{
					return variant.baker;
				}
			}

			MeshVariant newVariant;
			newVariant.baker = ImportMesh(rawMeshes.at(mesh), path, settings);
			newVariant.transform = Gleam::Float4x4::identity;
			variants.push_back(newVariant);
			return newVariant.baker;
		};

		auto HierarchyContainsNonUniformScaling = [&](auto self, const cgltf_node& node) -> bool
		{
			if (IsNonUniformScale(GetNodeTransform(node)))
			{
				return true;
			}

			for (uint32_t childIdx = 0; childIdx < node.children_count; ++childIdx)
			{
				const auto& childNode = node.children[childIdx];
				if (self(self, *childNode))
				{
					return true;
				}
			}
			return false;
		};

		auto world = Gleam::CreateRef<Gleam::World>(filename);
		auto ProcessNode = [&](auto self, const cgltf_node& node, const Gleam::TString& name,
			const Gleam::Float4x4& parentTransform, bool hierarchyHasNonUniformScaling) -> Gleam::EntityHandle
		{
			auto& entity = world->GetEntityManager().CreateEntity(name, Gleam::Guid::NewGuid());
			Gleam::Float4x4 nodeTransform = GetNodeTransform(node);
			Gleam::Float4x4 worldTransform = parentTransform * nodeTransform;

			if (hierarchyHasNonUniformScaling)
			{
				GLEAM_WARN("Node has non-uniform scaling. Baking transforms into geometry for its subtree: {0}", name);
			}
			else
			{
				Gleam::Float3 position;
				Gleam::Quaternion rotation;
				float scale;
				Gleam::Math::Decompose(nodeTransform, position, rotation, scale);
				entity.SetTranslation(position);
				entity.SetRotation(rotation);
				entity.SetScale(scale);
			}
			
			if (node.mesh)
			{
				auto meshBaker = GetOrCreateMesh(node.mesh, worldTransform, hierarchyHasNonUniformScaling);
				const auto& meshItem = Registry()->GetAsset<Gleam::MeshDescriptor>(meshBaker->Name());

				Gleam::TArray<Gleam::AssetReference> materialRefs;
				materialRefs.reserve(importedMaterials.size());
				for (const auto& materialBaker : importedMaterials)
				{
					const auto& materialItem = Registry()->GetAsset<Gleam::MaterialInstanceDescriptor>(materialBaker->Name());
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
					auto childHandle = self(self, *childNode, childName, worldTransform, hierarchyHasNonUniformScaling);
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
			bool hierarchyContainsNonUniformScaling = HierarchyContainsNonUniformScaling(HierarchyContainsNonUniformScaling, *root);
			auto entity = ProcessNode(ProcessNode, *root, entityName, Gleam::Float4x4::identity, hierarchyContainsNonUniformScaling);
		}
		else
		{
			auto& rootEntity = world->GetEntityManager().CreateEntity(filename, Gleam::Guid::NewGuid());

			uint32_t nodeIdx = 1;
			for (auto node : rootNodes)
			{
				bool hierarchyContainsNonUniformScaling = HierarchyContainsNonUniformScaling(HierarchyContainsNonUniformScaling, *node);
				auto entityName = GetNodeName(*node, filename + std::to_string(nodeIdx));
				auto entityHandle = ProcessNode(ProcessNode, *node, entityName, Gleam::Float4x4::identity, hierarchyContainsNonUniformScaling);
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

Gleam::RefCounted<MeshBaker> MeshSource::ImportMesh(const Gleam::TArray<RawMesh>& rawMeshes, const Gleam::Path& path, const ImportSettings& settings)
{
	static constexpr uint32_t kMaxVerticesPerMeshlet = MAX_MESHLET_VERTICES;
	static constexpr uint32_t kMaxTrianglesPerMeshlet = MAX_MESHLET_TRIANGLES;
	static constexpr float kConeWeight = 0.25f;
	
	MeshLodData lodData = MeshTools::CombineMeshes(rawMeshes);
	Gleam::TArray<Gleam::MeshletDescriptor> combinedMeshlets;
	Gleam::TArray<uint32_t> combinedMeshletVertices;
	Gleam::TArray<uint32_t> combinedMeshletTriangles;

	size_t totalIndexCount = 0;
	for (const auto& submesh : lodData.submeshes)
	{
		totalIndexCount += submesh.indexCount;
	}
	combinedMeshletTriangles.reserve(totalIndexCount / 3);
	combinedMeshletVertices.reserve(totalIndexCount);

	auto combinedIndices = lodData.indices.data();
	auto combinedPositions = lodData.positions.data();
	for (auto& submesh : lodData.submeshes)
	{
		Gleam::TArrayView<uint32_t> indices(combinedIndices + submesh.firstIndex, submesh.indexCount);
		Gleam::TArrayView<Gleam::Float3> positions(combinedPositions + submesh.baseVertex, submesh.vertexCount);

		meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), positions.size());

		size_t maxMeshlets = meshopt_buildMeshletsBound(indices.size(), kMaxVerticesPerMeshlet, kMaxTrianglesPerMeshlet);
		Gleam::TArray<meshopt_Meshlet> meshlets(maxMeshlets);
		Gleam::TArray<uint32_t> meshletVertices(indices.size());
		Gleam::TArray<uint8_t> meshletTriangleIndices(indices.size());
		size_t meshletCount = meshopt_buildMeshlets(meshlets.data(),
													meshletVertices.data(),
													meshletTriangleIndices.data(),
													indices.data(),
													indices.size(),
													(float*)positions.data(),
													positions.size(),
													sizeof(Gleam::Float3),
													kMaxVerticesPerMeshlet,
													kMaxTrianglesPerMeshlet,
													kConeWeight);

		const auto& last = meshlets[meshletCount - 1];
		meshletVertices.resize(last.vertex_offset + last.vertex_count);
		meshletTriangleIndices.resize(last.triangle_offset + last.triangle_count * 3);

		submesh.baseMeshlet = static_cast<uint32_t>(combinedMeshlets.size());
		submesh.meshletCount = static_cast<uint32_t>(meshletCount);
		combinedMeshlets.resize(combinedMeshlets.size() + meshletCount);
		for (uint32_t i = 0; i < meshletCount; ++i)
		{
			const auto& meshlet = meshlets[i];
			auto& meshletDesc = combinedMeshlets[submesh.baseMeshlet + i];

			auto meshletVerticesData = meshletVertices.data() + meshlet.vertex_offset;
			auto meshletTriangleData = meshletTriangleIndices.data() + meshlet.triangle_offset;
			
			meshopt_optimizeMeshlet(meshletVerticesData,
									meshletTriangleData,
									meshlet.triangle_count,
									meshlet.vertex_count);

			meshletDesc.vertexOffset = static_cast<uint32_t>(combinedMeshletVertices.size() + meshlet.vertex_offset);
			meshletDesc.triangleOffset = static_cast<uint32_t>(combinedMeshletTriangles.size());
			meshletDesc.vertexCount = static_cast<uint32_t>(meshlet.vertex_count);
			meshletDesc.triangleCount = static_cast<uint32_t>(meshlet.triangle_count);

			for (uint32_t t = 0; t < meshlet.triangle_count; ++t)
			{
				uint32_t packedTriangle = static_cast<uint32_t>(meshletTriangleData[t * 3 + 0])
										| (static_cast<uint32_t>(meshletTriangleData[t * 3 + 1]) << 8)
										| (static_cast<uint32_t>(meshletTriangleData[t * 3 + 2]) << 16);
				combinedMeshletTriangles.push_back(packedTriangle);
			}

			meshopt_Bounds bounds = meshopt_computeMeshletBounds(meshletVerticesData,
																 meshletTriangleData,
																 meshlet.triangle_count,
																 (float*)positions.data(),
																 positions.size(),
																 sizeof(Gleam::Float3));
			meshletDesc.coneApex = Gleam::Float3(bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2]);
			meshletDesc.coneAxis = Gleam::Float3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]);
			meshletDesc.coneCutoff = bounds.cone_cutoff;
			meshletDesc.center = Gleam::Float3(bounds.center[0], bounds.center[1], bounds.center[2]);
			meshletDesc.radius = bounds.radius;
		}

		combinedMeshletVertices.insert(combinedMeshletVertices.end(), meshletVertices.begin(), meshletVertices.end());
	}

	lodData.meshlets = std::move(combinedMeshlets);
	lodData.meshletVertices = std::move(combinedMeshletVertices);
	lodData.meshletTriangleIndices = std::move(combinedMeshletTriangles);

	return EmplaceBaker<MeshBaker>(std::move(lodData));
}

Gleam::TArray<Gleam::RefCounted<MaterialInstanceBaker>> MeshSource::ImportMaterials(const Gleam::TArray<RawMaterial>& rawMaterials, const Gleam::Path& path, const ImportSettings& settings)
{
	auto directory = path.Parent();
	auto opaqueLitMaterialAsset = AssetManager()->GetAsset<Gleam::MaterialDescriptor>("Materials/OpaqueLit").reference;
	auto maskLitMaterialAsset = AssetManager()->GetAsset<Gleam::MaterialDescriptor>("Materials/MaskLit").reference;
	auto transparentLitMaterialAsset = AssetManager()->GetAsset<Gleam::MaterialDescriptor>("Materials/TransparentLit").reference;

	auto assetManager = Gleam::Globals::GameInstance->GetSubsystem<Gleam::AssetManager>();
	auto opaqueLitMaterial = assetManager->LoadDescriptor<Gleam::MaterialDescriptor>(opaqueLitMaterialAsset);
	auto maskLitMaterial = assetManager->LoadDescriptor<Gleam::MaterialDescriptor>(maskLitMaterialAsset);
	auto transparentLitMaterial = assetManager->LoadDescriptor<Gleam::MaterialDescriptor>(transparentLitMaterialAsset);

	Gleam::TArray<Gleam::RefCounted<MaterialInstanceBaker>> materials;
	materials.reserve(rawMaterials.size());
	for (const auto& material : rawMaterials)
	{
		Gleam::MaterialInstanceDescriptor descriptor;
		descriptor.name = material.name;

		switch (material.alphaMode)
		{
			case Gleam::AlphaMode::Mask:
			{
				descriptor.material = maskLitMaterialAsset;
				descriptor.properties = maskLitMaterial.properties;
				break;
			}
			case Gleam::AlphaMode::Blend:
			{
				descriptor.material = transparentLitMaterialAsset;
				descriptor.properties = transparentLitMaterial.properties;
				break;
			}
			case Gleam::AlphaMode::Opaque:
			default:
			{
				descriptor.material = opaqueLitMaterialAsset;
				descriptor.properties = opaqueLitMaterial.properties;
				break;
			}
		}

		descriptor["BaseColor"] = material.albedoColor;
		descriptor["Emission"] = material.emissiveColor;
		descriptor["Metallic"] = material.metallicFactor;
		descriptor["Roughness"] = material.roughnessFactor;
		descriptor["OcclusionStrength"] = material.occlusionStrength;
		descriptor["AlphaCutoff"] = material.alphaCutoff;

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

		if (const auto& texture = material.textures[PBRTexture::Occlusion]; texture.Empty() == false)
		{
			auto texturePath = directory / texture;
			auto textureSettings = TextureSource::ImportSettings();
			textureSettings.generateMips = true;
			if ((texture == material.textures[PBRTexture::MetallicRoughness]) or ImportReference<TextureSource>(texturePath, textureSettings))
			{
				descriptor["OcclusionTexture"] = Registry()->GetAsset<Gleam::Texture2DDescriptor>(texture.Stem()).reference;
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
        else if (attribute.type == cgltf_attribute_type_texcoord && attribute.index == 0) // TEXCOORD_0
        {
            mesh.texCoords.resize(vertexCount);
            cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)mesh.texCoords.data(), mesh.texCoords.size() * 2);
        }
        else if (attribute.type == cgltf_attribute_type_color && attribute.index == 0) // COLOR_0
        {
            mesh.colors.resize(vertexCount, Gleam::Float4::one);
            if (cgltf_num_components(attribute.data->type) == cgltf_type_vec4)
            {
                cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)mesh.colors.data(), mesh.colors.size() * 4);
            }
            else
            {
                Gleam::TArray<Gleam::Float3> rgb(vertexCount);
                cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)rgb.data(), rgb.size() * 3);
                for (uint32_t i = 0; i < vertexCount; ++i)
                {
                    mesh.colors[i] = Gleam::Float4(rgb[i].x, rgb[i].y, rgb[i].z, 1.0f);
                }
            }
        }
    }
	MeshTools::RemoveDegenerateFaces(mesh);
	
	if (mesh.normals.empty())
	{
		MeshTools::ComputeSmoothNormals(mesh);
	}

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
	MeshTools::ValidateTangents(mesh);

	if (mesh.texCoords.empty())
	{
		mesh.texCoords.resize(mesh.positions.size(), Gleam::Float2::zero);
	}

	if (mesh.colors.empty())
	{
		mesh.colors.resize(mesh.positions.size(), Gleam::Float4::one);
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
    
    // Occlusion
    if (auto texture = mat.occlusion_texture.texture; texture != nullptr)
    {
		Gleam::Path file = texture->image->uri;
        material.textures[PBRTexture::Occlusion] = file;
        material.occlusionStrength = mat.occlusion_texture.scale;
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
            material.alphaMode = Gleam::AlphaMode::Opaque;
            break;
        }
        case cgltf_alpha_mode_mask:
        {
            material.alphaMode = Gleam::AlphaMode::Mask;
            break;
        }
        case cgltf_alpha_mode_blend:
        {
            material.alphaMode = Gleam::AlphaMode::Blend;
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

Gleam::Float4x4 ConvertRHtoLH(const Gleam::Float4x4& transform)
{
	// glTF is right-handed with +Z toward the viewer, the engine is left-handed with +Z forward
	const auto flipZ = Gleam::Float4x4::Scale(Gleam::Float3(1.0f, 1.0f, -1.0f));
	return flipZ * transform * flipZ;
}

void ConvertRHtoLH(RawMesh& mesh)
{
	MeshTools::ApplyTransform(mesh, Gleam::Float4x4::Scale(Gleam::Float3(1.0f, 1.0f, -1.0f)));
}

Gleam::Float4x4 GetNodeTransform(const cgltf_node& node)
{
	if (node.has_matrix)
	{
		return ConvertRHtoLH(Gleam::Float4x4((float*)node.matrix));
	}

	Gleam::Float3 position = Gleam::Float3::zero;
	Gleam::Quaternion rotation = Gleam::Quaternion::identity;
	Gleam::Float3 scale = Gleam::Float3::one;

	if (node.has_translation)
	{
		position = Gleam::Float3(node.translation[0], node.translation[1], node.translation[2]);
	}

	if (node.has_rotation)
	{
		rotation = Gleam::Quaternion(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
	}

	if (node.has_scale)
	{
		scale = Gleam::Float3(node.scale[0], node.scale[1], node.scale[2]);
	}
	return ConvertRHtoLH(Gleam::Float4x4::TRS(position, rotation, scale));
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

bool IsSameTransform(const Gleam::Float4x4& lhs, const Gleam::Float4x4& rhs)
{
	for (uint32_t i = 0; i < 16; ++i)
	{
		if (Gleam::Math::Abs(lhs.m[i] - rhs.m[i]) >= Gleam::Math::Epsilon)
		{
			return false;
		}
	}
	return true;
}

bool IsNonUniformScale(const Gleam::Float4x4& transform)
{
	const Gleam::Float3 xAxis(transform.m[0], transform.m[1], transform.m[2]);
	const Gleam::Float3 yAxis(transform.m[4], transform.m[5], transform.m[6]);
	const Gleam::Float3 zAxis(transform.m[8], transform.m[9], transform.m[10]);

	const float lengthX = Gleam::Math::Length(xAxis);
	const float lengthY = Gleam::Math::Length(yAxis);
	const float lengthZ = Gleam::Math::Length(zAxis);
	const float determinant = Gleam::Math::Dot(Gleam::Math::Cross(xAxis, yAxis), zAxis);
	return determinant < 0.0f
		|| Gleam::Math::Abs(lengthX - lengthY) >= Gleam::Math::Epsilon
		|| Gleam::Math::Abs(lengthX - lengthZ) >= Gleam::Math::Epsilon;
}
