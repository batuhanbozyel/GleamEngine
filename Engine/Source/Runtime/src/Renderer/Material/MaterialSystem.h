//
//  MaterialSystem.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.07.2023.
//

#pragma once
#include "Core/Subsystem.h"
#include "Material.h"

namespace Gleam {

struct PBRMaterialData
{
    TString baseTexture;
};

class MaterialSystem final : public Subsystem
{
public:

	const RefCounted<Material>& CreateMaterial(const MaterialDescriptor& descriptor);

private:

	HashMap<MaterialDescriptor, RefCounted<Material>> mMaterials;

};

} // namespace Gleam
