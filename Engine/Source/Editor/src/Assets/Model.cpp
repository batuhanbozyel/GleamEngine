#include "Gleam.h"
#include "Model.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace GEditor;

static uint32_t InsertUniqueMaterial(Gleam::TArray<RawMaterial>& materials, const RawMaterial& material);
static RawMesh ProcessAttributes(const cgltf_primitive& primitive);
static RawMaterial ProcessMaterial(const cgltf_material& material);

bool Model::Import(const Gleam::Filesystem::path& path, const ImportSettings& settings)
{
	Gleam::TString gltf_path = path.string();
	cgltf_options options = {};
	cgltf_data* data = nullptr;

	cgltf_result result = cgltf_parse_file(&options, gltf_path.c_str(), &data);
	if (result != cgltf_result_success)
	{
		return false;
	}

	result = cgltf_load_buffers(&options, data, gltf_path.c_str());
	if (result != cgltf_result_success)
	{
        cgltf_free(data);
		return false;
	}
    
    for(uint32_t i = 0; i < data->meshes_count; ++i)
    {
        const auto& mesh = data->meshes[i];
        for(uint32_t meshIdx = 0; meshIdx < mesh.primitives_count; ++meshIdx)
        {
            auto rawMesh = ProcessAttributes(mesh.primitives[meshIdx]);
            rawMesh.name = mesh.name;
            
            RawMaterial material;
            if (auto mat = mesh.primitives[meshIdx].material; mat != nullptr)
            {
                material = ProcessMaterial(*mat);
            }
            rawMesh.materialIndex = InsertUniqueMaterial(materials, material);
            
            meshes.push_back(rawMesh);
        }
    }
    
	cgltf_free(data);
    return true;
}

RawMesh ProcessAttributes(const cgltf_primitive& primitive)
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
        mesh.normals.resize(vertex_count, Gleam::Vector3(0.5f, 0.5f, 1.0f));
    }
    
    if (mesh.texCoords.empty())
    {
        mesh.texCoords.resize(vertex_count, Gleam::Vector2::zero);
    }
    return mesh;
}

RawMaterial ProcessMaterial(const cgltf_material& mat)
{
    RawMaterial material;
    material.name = mat.name;
    
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
            material.textures[RawTexture::Albedo].name = texture->image->name;
            material.textures[RawTexture::Albedo].file = texture->image->uri;
        }
        
        if (auto texture = pbr.metallic_roughness_texture.texture; texture != nullptr)
        {
            material.textures[RawTexture::MetallicRoughness].name = texture->image->name;
            material.textures[RawTexture::MetallicRoughness].file = texture->image->uri;
        }
    }
    
    // Normal
    if (auto texture = mat.normal_texture.texture; texture != nullptr)
    {
        material.textures[RawTexture::Normal].name = texture->image->name;
        material.textures[RawTexture::Normal].file = texture->image->uri;
    }
    
    // Emissive
    if (auto texture = mat.emissive_texture.texture; texture != nullptr)
    {
        material.textures[RawTexture::Emissive].name = texture->image->name;
        material.textures[RawTexture::Emissive].file = texture->image->uri;
    }
    
    float emissiveStrength = mat.has_emissive_strength ? mat.emissive_strength.emissive_strength : 1.0f;
    material.emissiveColor = Gleam::Color(mat.emissive_factor[0] * emissiveStrength,
                                          mat.emissive_factor[1] * emissiveStrength,
                                          mat.emissive_factor[2] * emissiveStrength,
                                          1.0f);
    
    // Occlusion
    if (auto texture = mat.occlusion_texture.texture; texture != nullptr)
    {
        material.textures[RawTexture::Occlusion].name = texture->image->name;
        material.textures[RawTexture::Occlusion].file = texture->image->uri;
    }
    
    material.doubleSided = mat.double_sided;
    material.alphaCutoff = mat.alpha_cutoff;
    material.unlit = mat.unlit;
    
    switch (mat.alpha_mode)
    {
        case cgltf_alpha_mode_opaque:
        {
            material.alphaBlend = Gleam::RenderQueue::Opaque;
            break;
        }
        case cgltf_alpha_mode_blend:
        {
            material.alphaBlend = Gleam::RenderQueue::Transparent;
            break;
        }
        case cgltf_alpha_mode_mask:
        {
            GLEAM_ASSERT(false, "Not implemented yet!");
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

uint32_t InsertUniqueMaterial(Gleam::TArray<RawMaterial>& materials, const RawMaterial& material)
{
    auto beginIt = materials.begin();
    auto materialIt = std::find(materials.begin(), materials.end(), material);
    if (materialIt == materials.end())
    {
        materials.push_back(material);
    }
    return static_cast<uint32_t>(std::distance(beginIt, materialIt));
}
