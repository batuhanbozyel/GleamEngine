//
//  MaterialInstance.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.04.2023.
//

#pragma once
#include "IMaterial.h"

namespace Gleam {

class MaterialInstance : public IMaterial
{
public:

    MaterialInstance(const IMaterial* material, uint32_t uniqueId);
    
    void SetProperty(const TString& name, const MaterialPropertyValue& value);

    const IMaterial* GetBaseMaterial() const;
    
    uint32_t GetUniqueId() const;
    
private:
    
    uint32_t mUniqueId = 0;
    
	const IMaterial* mBaseMaterial = nullptr;

	TArray<MaterialPropertyValue> mPropertyValues;
    
};

} // namespace Gleam
