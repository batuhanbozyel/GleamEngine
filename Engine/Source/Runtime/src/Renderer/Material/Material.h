//
//  Material.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "IMaterial.h"
#include "MaterialInstance.h"
#include "MaterialDescriptor.h"

namespace Gleam {

class Shader;

struct MaterialPass
{
    PipelineStateDescriptor pipelineState;
    RefCounted<Shader> vertexFunction;
    RefCounted<Shader> fragmentFunction;
};

class Material : public IMaterial
{
public:
    
    Material(const MaterialDescriptor& descriptor);
    
    RefCounted<MaterialInstance> CreateInstance();
    
    RenderQueue GetRenderQueue() const;

	const TArray<MaterialPass>& GetPasses() const;
    
private:
    
    uint32_t mInstanceCount = 0;
    
    RenderQueue mRenderQueue;

	TArray<MaterialPass> mPasses;
    
};

} // namespace Gleam
