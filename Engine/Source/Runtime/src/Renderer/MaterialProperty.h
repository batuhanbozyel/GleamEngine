//
//  MaterialProperty.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once

namespace Gleam {

class Buffer;
class Texture;
class Texture2D;
class TextureCube;

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

using MaterialPropertyValue = std::variant<float, Vector2, Vector3, Vector4, RefCounted<Buffer>, RefCounted<Texture2D>, RefCounted<TextureCube>>;

struct MaterialProperty
{
    TString name;
    MaterialPropertyValue value;
    MaterialPropertyType type;
};

} // namespace Gleam