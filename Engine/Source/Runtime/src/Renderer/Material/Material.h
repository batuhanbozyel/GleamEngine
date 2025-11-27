//
//  Material.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "IMaterial.h"
#include "MaterialDescriptor.h"

#include "Renderer/Buffer.h"
#include "Renderer/ResourceDescriptorHeap.h"

namespace Gleam {

class MaterialInstance;

class Material : public IMaterial
{
	static constexpr uint32_t MaxMaterialInstances = 1024;
public:
    
    Material(const MaterialDescriptor& descriptor);

	~Material();

	ShaderResourceIndex CreateInstance(const TArray<MaterialPropertyValue>& values);

	void DestroyInstance(ShaderResourceIndex& instance);
    
    const Buffer& GetBuffer() const;
    
    const TString& GetName() const;

	uint32_t GetPipelineHash() const;

	uint32_t GetInstanceCount() const;
    
private:
    
    TString mName;

    Buffer mBuffer;

	size_t mInstanceSize = 0;

	uint32_t mPipelineStateHash = 0;

	ResourceDescriptorHeap mInstanceDescriptorHeap;
    
};

} // namespace Gleam
