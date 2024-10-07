//
//  MaterialInstance.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.04.2023.
//

#pragma once
#include "IMaterial.h"

namespace Gleam {

struct MaterialInstanceDescriptor;

class MaterialInstance : public IMaterial
{
public:

    MaterialInstance(const MaterialInstanceDescriptor& descriptor);
    
	void SetProperty(const TString& name, const MaterialPropertyValue& value);

	const AssetReference& GetBaseMaterial() const;
    
    uint32_t GetUniqueId() const;
    
private:
    
    uint32_t mUniqueId = 0;
    
	AssetReference mBaseMaterial;

	TArray<MaterialPropertyValue> mPropertyValues;
    
};

} // namespace Gleam
