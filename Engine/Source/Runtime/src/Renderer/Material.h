//
//  Material.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "Shader.h"
#include "MaterialProperty.h"
#include "PipelineStateDescriptor.h"

namespace Gleam {

class MaterialInstance;

enum class RenderQueue
{
    Opaque,
    Transparent
};

struct MaterialPass
{
    PipelineStateDescriptor pipelineState;
    RefCounted<Shader> vertexFunction;
    RefCounted<Shader> fragmentFunction;
};

struct MaterialDescriptor
{
    TArray<MaterialProperty> properties;
    TArray<MaterialPass> passes;
    RenderQueue renderQueue;
};

class Material : public std::enable_shared_from_this<Material>
{
public:
    
    GLEAM_NONCOPYABLE(Material);
    
    Material(const MaterialDescriptor& descriptor);
    
    RefCounted<MaterialInstance> CreateInstance();
    
    const TArray<MaterialPass>& GetPasses() const;
    
    const TArray<MaterialProperty>& GetProperties() const;
    
    RenderQueue GetRenderQueue() const;
    
    uint32_t GetPropertyIndex(const TString& name) const;
    
private:
    
    uint32_t mInstanceCount = 0;
    
    MaterialDescriptor mDescriptor;
    
};

} // namespace Gleam
