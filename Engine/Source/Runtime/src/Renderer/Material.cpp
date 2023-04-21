//
//  Material.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#include "gpch.h"
#include "Material.h"

using namespace Gleam;

Material::Material(const TArray<MaterialProperty>& properties, const PipelineStateDescriptor& pipelineState)
	: mProperties(properties), mPipelineState(pipelineState)
{

}

MaterialProperty* Material::GetProperty(const TString& name)
{
    for (auto& property : mProperties)
    {
        if (property.name == name) { return &property; }
    }
    GLEAM_ASSERT(false, "Material property with name could not found!");
    return nullptr;
}

void Material::SetProperty(const TString& name, float value)
{
    auto property = GetProperty(name);
    if (property)
    {
        GLEAM_ASSERT(property->type == MaterialPropertyType::Scalar, "Material property is not a scalar type!");
        property->value = value;
    }
}

void Material::SetProperty(const TString& name, const Vector2& value)
{
    auto property = GetProperty(name);
    if (property)
    {
        GLEAM_ASSERT(property->type == MaterialPropertyType::Vector2, "Material property is not a 2d vector type!");
        property->value = value;
    }
}

void Material::SetProperty(const TString& name, const Vector3& value)
{
    auto property = GetProperty(name);
    if (property)
    {
        GLEAM_ASSERT(property->type == MaterialPropertyType::Vector3, "Material property is not a 3d vector type!");
        property->value = value;
    }
}

void Material::SetProperty(const TString& name, const Vector4& value)
{
    auto property = GetProperty(name);
    if (property)
    {
        GLEAM_ASSERT(property->type == MaterialPropertyType::Vector4, "Material property is not a 4d vector type!");
        property->value = value;
    }
}

void Material::SetProperty(const TString& name, const RefCounted<Buffer>& buffer)
{
    auto property = GetProperty(name);
    if (property)
    {
        GLEAM_ASSERT(property->type == MaterialPropertyType::Buffer, "Material property is not a buffer!");
        property->value = buffer;
    }
}

void Material::SetProperty(const TString& name, const RefCounted<Texture2D>& texture2d)
{
    auto property = GetProperty(name);
    if (property)
    {
        GLEAM_ASSERT(property->type == MaterialPropertyType::Texture2D, "Material property is not a texture2d!");
        property->value = texture2d;
    }
}

void Material::SetProperty(const TString& name, const RefCounted<TextureCube>& textureCube)
{
    auto property = GetProperty(name);
    if (property)
    {
        GLEAM_ASSERT(property->type == MaterialPropertyType::TextureCube, "Material property is not a textureCube!");
        property->value = textureCube;
    }
}
