#pragma once
#include "Gleam.h"

namespace GEditor {

enum class AssetType
{
    TextureSource,
    MeshSource,
    Font
};

static Gleam::TArray<Gleam::TStringView> AssetTypeSupportedExtensions(AssetType type)
{
    switch (type)
    {
        case AssetType::TextureSource: return {""};
        case AssetType::MeshSource: return {"gltf"};
        case AssetType::Font: return {""};
        default: return {""};
    }
}

} // namespace GEditor
