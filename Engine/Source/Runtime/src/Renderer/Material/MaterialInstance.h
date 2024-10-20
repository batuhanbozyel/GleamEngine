//
//  MaterialInstance.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.04.2023.
//

#pragma once
#include "IMaterial.h"
#include "MaterialDescriptor.h"

namespace Gleam {

class MaterialInstance : public IMaterial
{
public:

    MaterialInstance(const MaterialInstanceDescriptor& descriptor);

	virtual void Release() override;
    
	void SetProperty(const TString& name, const MaterialPropertyValue& value);

	const AssetReference& GetBaseMaterial() const;
    
	uint32_t GetID() const;
    
private:
    
	ShaderResourceIndex mResourceView = InvalidResourceIndex;
    
	AssetReference mBaseMaterial;

	TArray<MaterialPropertyValue> mPropertyValues;
    
};

} // namespace Gleam
