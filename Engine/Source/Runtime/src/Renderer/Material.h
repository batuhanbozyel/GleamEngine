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

class Material
{
public:

    Material(const TArray<MaterialProperty>& properties, const PipelineStateDescriptor& pipelineState);
    
    void SetProperty(const TString& name, float value);
    
    void SetProperty(const TString& name, const Vector2& value);
    
    void SetProperty(const TString& name, const Vector3& value);
    
    void SetProperty(const TString& name, const Vector4& value);
    
    void SetProperty(const TString& name, const RefCounted<Buffer>& buffer);
    
    void SetProperty(const TString& name, const RefCounted<Texture2D>& texture2d);
    
    void SetProperty(const TString& name, const RefCounted<TextureCube>& textureCube);
    
private:
    
    MaterialProperty* GetProperty(const TString& name);
    
    TArray<MaterialProperty> mProperties;

	PipelineStateDescriptor mPipelineState;
    
};

} // namespace Gleam
