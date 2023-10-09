//
//  MaterialProperty.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include <variant>
#include "Renderer/Buffer.h"
#include "Renderer/Texture.h"

namespace Gleam {

enum class MaterialPropertyType
{
    Scalar,
    Vector2,
    Vector3,
    Vector4,
    Buffer,
    Texture2D,
    TextureCube
};

using MaterialPropertyValue = std::variant<float, Vector2, Vector3, Vector4, Buffer, Texture>;

struct MaterialProperty
{
    TString name;
    MaterialPropertyType type;

    bool operator==(const MaterialProperty& other) const
    {
        return name == other.name && type == other.type;
    }
};

} // namespace Gleam

template <>
struct std::hash<Gleam::MaterialProperty>
{
    size_t operator()(const Gleam::MaterialProperty& property) const
    {
        std::size_t hash = 0;
        Gleam::hash_combine(hash, std::hash<Gleam::TString>()(property.name));
        Gleam::hash_combine(hash, std::hash<int>()(static_cast<int>(property.type)));
        return hash;
    }
};
