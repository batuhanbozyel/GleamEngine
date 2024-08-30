#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

struct RawTexture
{
    Gleam::TString name = "";
    Gleam::Filesystem::Path file = "";
    
    enum Type
    {
        Albedo,
        MetallicRoughness,
        Normal,
        Emissive,
        Occlusion,
        COUNT
    };
    
    bool operator==(const RawTexture& other) const
    {
        return name == other.name;
    }
};

struct RawMaterial
{
    Gleam::TString name = "";
    Gleam::TArray<RawTexture, RawTexture::COUNT> textures;
    Gleam::Color albedoColor = Gleam::Color::white;
    Gleam::Color emissiveColor = Gleam::Color::clear;
    float alphaCutoff = 0.5f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    bool doubleSided = false;
    bool alphaBlend = false;
    bool unlit = false;
    
    bool operator==(const RawMaterial& other) const
    {
        return name == other.name
            && textures == other.textures
            && albedoColor == other.albedoColor
            && emissiveColor == other.emissiveColor
            && alphaCutoff == other.alphaCutoff
            && metallicFactor == other.metallicFactor
            && roughnessFactor == other.roughnessFactor
            && doubleSided == other.doubleSided
            && alphaBlend == other.alphaBlend
            && unlit == other.unlit;
    }
    
    bool operator!=(const RawMaterial& other) const
    {
        return !(*this == other);
    }
};

struct RawMesh
{
    Gleam::TString name;
    Gleam::TArray<Gleam::Float3> positions;
    Gleam::TArray<Gleam::Float3> normals;
    Gleam::TArray<Gleam::Float2> texCoords;
    Gleam::TArray<uint32_t> indices;
    uint32_t materialIndex = 0;
};

struct MeshSource : AssetPackage
{
    struct ImportSettings
    {
        bool combineMeshes = false;
    };
    
	/*
	* glTF file requirements:
	*	- position, normal, uv attributes
	*	- triangulated primitive type and indices
	*/
	bool Import(const Gleam::Filesystem::Path& path, const ImportSettings& settings);
};

} // namespace GEditor
