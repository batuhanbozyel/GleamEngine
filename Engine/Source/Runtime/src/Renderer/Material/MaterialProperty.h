//
//  MaterialProperty.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "Renderer/Buffer.h"
#include "Renderer/Texture.h"

#include <variant>

namespace Gleam {

enum class MaterialPropertyType
{
    Scalar,
    Float2,
    Float3,
    Float4,
    Texture2D
};

using MaterialPropertyValue = std::variant<float, Float2, Float3, Float4, Texture>;

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
        Gleam::hash_combine(hash, property.name);
        Gleam::hash_combine(hash, property.type);
        return hash;
    }
};

GLEAM_TYPE(Gleam::MaterialProperty, Guid("A69E7110-6B1B-41B9-ACFB-AA363C9A0943"))
	GLEAM_FIELD(name, Serializable())
	GLEAM_FIELD(type, Serializable())
GLEAM_END
