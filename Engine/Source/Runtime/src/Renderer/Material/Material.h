//
//  Material.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "IMaterial.h"
#include "MaterialDescriptor.h"

#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"
#include "Renderer/ResourceDescriptorHeap.h"

namespace Gleam {

class MaterialInstance;

class Material : public IMaterial
{
public:
    
    Material(const MaterialDescriptor& descriptor);

	ShaderResourceIndex CreateInstance(const TArray<MaterialPropertyValue>& values);

	void DestroyInstance(ShaderResourceIndex& instance);
    
	virtual void Release() override;
    
    const Buffer& GetBuffer() const;
    
    const TString& GetName() const;

	uint32_t GetPipelineHash() const;
    
private:
    
    TString mName;
    
	Heap mHeap;

    Buffer mBuffer;

	size_t mInstanceSize = 0;

	uint32_t mPipelineStateHash = 0;

	ResourceDescriptorHeap mInstanceDescriptorHeap;

	static constexpr uint32_t MaxMaterialInstances = 1000;
    
};

} // namespace Gleam
