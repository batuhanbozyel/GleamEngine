#pragma once
#include "Core/GUID.h"

namespace Gleam {

struct SubmeshDescriptor
{
    BoundingBox bounds;
    uint32_t baseVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
};

struct MeshDescriptor
{
    TString name;
    TArray<uint32_t> indices;
    TArray<Float3> positions;
    TArray<InterleavedMeshVertex> interleavedVertices;
    TArray<SubmeshDescriptor> submeshes;
};

} // namespace Gleam

GLEAM_TYPE(Gleam::SubmeshDescriptor, Guid("DD7E3A74-ADF4-45A9-8DFD-CA252EDC49A6"))
	GLEAM_FIELD(bounds, Serializable())
    GLEAM_FIELD(baseVertex, Serializable())
    GLEAM_FIELD(firstIndex, Serializable())
    GLEAM_FIELD(indexCount, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::InterleavedMeshVertex, Guid("4AFE936A-550F-419C-A7F0-5ED38D9D1642"))
    GLEAM_FIELD(normal, Serializable())
    GLEAM_FIELD(texCoord, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::MeshDescriptor, Guid("59E4007E-F7D4-4107-A05F-E1121067DCD3"))
    GLEAM_FIELD(name, Serializable())
    GLEAM_FIELD(indices, Serializable())
    GLEAM_FIELD(positions, Serializable())
    GLEAM_FIELD(interleavedVertices, Serializable())
    GLEAM_FIELD(submeshes, Serializable())
GLEAM_END
