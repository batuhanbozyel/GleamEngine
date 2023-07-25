//
//  MaterialInstance.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.04.2023.
//

#pragma once

namespace Gleam {

class Material;

class MaterialInstance
{
    friend class Material;
    
public:
    
    MaterialInstance(const RefCounted<Material>& material, uint32_t uniqueId);
    
    void SetProperty(const TString& name, const MaterialPropertyValue& value);
    
    const RefCounted<Material>& GetMaterial() const;
    
    uint32_t GetUniqueId() const;
    
private:
    
    uint32_t mUniqueId;
    
    RefCounted<Material> mMaterial;
    
    TArray<MaterialPropertyValue> mPropertyValues;
    
};

} // namespace Gleam
