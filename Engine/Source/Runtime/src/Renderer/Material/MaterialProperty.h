//
//  MaterialProperty.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "Assets/AssetReference.h"

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

using MaterialPropertyValue = std::variant<float, Float2, Float3, Float4, AssetReference>;

struct MaterialProperty
{
    TString name;
    MaterialPropertyType type;
};

} // namespace Gleam

GLEAM_ENUM(Gleam::MaterialPropertyType, Guid("C8CF1F83-7A80-4156-BA7B-947208CB69B6"))
GLEAM_TYPE(Gleam::MaterialProperty, Guid("A69E7110-6B1B-41B9-ACFB-AA363C9A0943"))
	GLEAM_FIELD(name, Serializable())
	GLEAM_FIELD(type, Serializable())
GLEAM_END
