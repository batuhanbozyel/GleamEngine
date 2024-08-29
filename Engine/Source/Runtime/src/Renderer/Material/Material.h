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

class Material : public IMaterial
{
public:
    
    Material(const MaterialDescriptor& descriptor);
    
    RefCounted<MaterialInstance> CreateInstance();

	const Shader& GetShader() const;

	size_t GetPipelineHash() const;
    
    RenderQueue GetRenderQueue() const;
    
private:
    
	Shader mShader;
    
    RenderQueue mRenderQueue;

	size_t mPipelineStateHash;

    uint32_t mInstanceCount = 0;
    
};

} // namespace Gleam
