#pragma once

namespace Gleam {
    
enum class FileType
{
    Image,
    Model,
    Font
};

static TArray<TStringView> FileTypeSupportedExtensions(FileType type)
{
    switch (type)
    {
        case FileType::Image: return {""};
        case FileType::Model: return {"obj"};
        case FileType::Font: return {""};
        default: return {""};
    }
}

} // namespace Gleam
