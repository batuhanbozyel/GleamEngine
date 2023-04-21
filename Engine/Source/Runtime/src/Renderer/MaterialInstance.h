//
//  MaterialInstance.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.04.2023.
//

#pragma once
#include "Material.h"

namespace Gleam {

class MaterialInstance
{
public:
    
    MaterialInstance(const RefCounted<Material>& parentMaterial);
    
private:
    
    RefCounted<Material> mParentMaterial;
    
};

} // namespace Gleam