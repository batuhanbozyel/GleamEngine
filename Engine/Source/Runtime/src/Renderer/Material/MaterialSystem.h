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

class MaterialSystem final : public Subsystem
{
public:

	virtual void Initialize() override;

	virtual void Shutdown() override;

	RefCounted<Material> GetDefaultMaterial();

	RefCounted<Material> CreateMaterial(const MaterialDescriptor& descriptor);

private:

	RefCounted<Material> mDefaultMaterial;

    HashMap<MaterialDescriptor, RefCounted<Material>> mMaterials;

};

} // namespace Gleam
