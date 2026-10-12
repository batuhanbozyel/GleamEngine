#pragma once
#include "AssetPackage.h"
#include "Tools/MeshTools.h"
#include "Core/Attributes.h"
#include "Container/EnumFlag.h"
#include "Math/Color.h"
#include "Math/Quaternion.h"
#include "Math/Float4x4.h"
#include "Renderer/Material/MaterialDescriptor.h"

namespace GEditor {

class MeshBaker;
class MaterialInstanceBaker;

struct PBRTexture
{
    enum Type
    {
        Albedo,
        Normal,
        MetallicRoughness,
        Occlusion,
        Emissive,
        COUNT
    };
};

struct RawMaterial
{
    Gleam::TString name = "Default";
    Gleam::TArray<Gleam::Path, PBRTexture::COUNT> textures;
    Gleam::Color albedoColor = Gleam::Color::white;
    Gleam::Color emissiveColor = Gleam::Color::clear;
    float alphaCutoff = 0.5f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float occlusionStrength = 1.0f;
    bool doubleSided = false;
    Gleam::AlphaMode alphaMode = Gleam::AlphaMode::Opaque;
    bool unlit = false;

    bool operator==(const RawMaterial& other) const
    {
        return textures == other.textures
            && albedoColor == other.albedoColor
            && emissiveColor == other.emissiveColor
            && alphaCutoff == other.alphaCutoff
            && metallicFactor == other.metallicFactor
            && roughnessFactor == other.roughnessFactor
            && occlusionStrength == other.occlusionStrength
            && doubleSided == other.doubleSided
            && alphaMode == other.alphaMode
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
	Gleam::BoundingBox aabb;
    Gleam::TArray<Gleam::Float3> positions;
    Gleam::TArray<Gleam::Float3> normals;
	Gleam::TArray<Gleam::Float4> tangents;
    Gleam::TArray<Gleam::Float2> texCoords;
    Gleam::TArray<Gleam::Float4> colors;
    Gleam::TArray<uint32_t> indices;
	uint32_t material;
};

GENUM(MeshColliderType, "C0F303DD-F32E-4BD0-91B0-2D8C3972EE97", Serializable, PrettyName("Collider Type")) : uint32_t
{
	GITEM(ConvexHull, "B09BAE6B-8BA6-41B1-B7EE-74319F454E67", PrettyName("Convex Hull")) = BIT(0),
	GITEM(TriangleMesh, "B7D58578-6574-4149-8F19-71671B2ECD4C", PrettyName("Triangle Mesh")) = BIT(1)
};

GSTRUCT(MeshPhysicsImportSettings, "7E40A63F-A2AD-4892-936C-8D9FD2DDFDB5", Serializable, PrettyName("Physics"))
{
	GFIELD("67BF0F77-BDEF-46C8-9DAC-BBC757F16F0D", Serializable, PrettyName("Colliders"))
	Gleam::EnumFlag<MeshColliderType> colliders = Gleam::EnumFlag<MeshColliderType>(MeshColliderType::ConvexHull) | MeshColliderType::TriangleMesh;

	GFIELD("20E25FE6-1E88-4A8D-98E7-570335777441", Serializable, PrettyName("Convex Decomposition"))
	ConvexDecompositionSettings convexDecomposition;

	GFIELD("EC464104-92DC-4C5A-9279-526F74FC984C", Serializable, PrettyName("Triangle Mesh Simplification"))
	TriangleMeshSimplificationSettings triangleMeshSimplification;
};

GSTRUCT(MeshImportSettings, "23C5B89C-5735-4CFA-BDD9-0E556AEB0241", Serializable, PrettyName("Mesh"))
{
	GFIELD("4854FDA0-E1F1-444F-95F4-AC748D0CDB7A", Serializable, PrettyName("Generate LODs"))
	bool generateLods = true;

	GFIELD("94883C7C-3AF3-46BA-89A6-D590416F589D", Serializable, PrettyName("Physics"))
	MeshPhysicsImportSettings physics;
};

class MeshSource : public AssetPackage
{
public:
	AssetPackageType(MeshSource);

	using ImportSettings = MeshImportSettings;

	/*
	* glTF file requirements:
	*	- position, normal, uv attributes
	*	- triangulated primitive type and indices
	*/
	bool Import(const Gleam::Path& path, const ImportSettings& settings);

private:

	Gleam::RefCounted<MeshBaker> ImportMesh(const Gleam::TArray<RawMesh>& rawMeshes, const Gleam::Path& path, const ImportSettings& settings);

	Gleam::TArray<Gleam::RefCounted<MaterialInstanceBaker>> ImportMaterials(const Gleam::TArray<RawMaterial>& rawMaterials, const Gleam::Path& path, const ImportSettings& settings);
    
};

} // namespace GEditor
