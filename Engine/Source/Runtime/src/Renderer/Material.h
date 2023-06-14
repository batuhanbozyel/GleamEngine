//
//  Material.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "MaterialProperty.h"
#include "PipelineStateDescriptor.h"

namespace Gleam {

class Shader;

class Material
{
public:
    
    static Material Import(const Filesystem::path& path);
    
    Material(const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, const TArray<MaterialProperty>& properties);
    
    void SetProperty(const TString& name, float value);
    
    void SetProperty(const TString& name, const Vector2& value);
    
    void SetProperty(const TString& name, const Vector3& value);
    
    void SetProperty(const TString& name, const Vector4& value);
    
    void SetProperty(const TString& name, const RefCounted<Buffer>& buffer);
    
    void SetProperty(const TString& name, const RefCounted<Texture2D>& texture2d);
    
    void SetProperty(const TString& name, const RefCounted<TextureCube>& textureCube);
    
    const TArray<MaterialProperty>& GetProperties() const;
    
private:
    
    MaterialProperty* GetProperty(const TString& name);
    
    TArray<MaterialProperty> mProperties;

	PipelineStateDescriptor mPipelineState;
    
    RefCounted<Shader> mVertexShader;
    
    RefCounted<Shader> mFragmentShader;
    
};

} // namespace Gleam
