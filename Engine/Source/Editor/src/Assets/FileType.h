#pragma once
#include "Gleam.h"

namespace GEditor {
    
enum class FileType
{
    Image,
    Model,
    Font
};

static Gleam::TArray<Gleam::TStringView> FileTypeSupportedExtensions(FileType type)
{
    switch (type)
    {
        case FileType::Image: return {""};
        case FileType::Model: return {"gltf"};
        case FileType::Font: return {""};
        default: return {""};
    }
}

} // namespace GEditor
