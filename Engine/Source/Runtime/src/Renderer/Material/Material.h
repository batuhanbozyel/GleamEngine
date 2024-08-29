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

	uint32_t GetPipelineHash() const;
    
    RenderQueue GetRenderQueue() const;
    
private:
    
	Shader mShader;
    
    RenderQueue mRenderQueue;

	uint32_t mPipelineStateHash = 0;

    uint32_t mInstanceCount = 0;
    
};

} // namespace Gleam
