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

	Material* Get(const TString& name) const;

private:

	struct MaterialComparator
	{
		bool operator()(const Scope<Material>& m1, const Scope<Material>& m2) const
		{
			return m1->GetName() == m2->GetName();
		}
	};

	struct MaterialHasher
	{
		size_t operator()(const Scope<Material>& material) const
		{
			return std::hash<TString>()(material->GetName());
		}
	};

	HashSet<Scope<Material>, MaterialHasher, MaterialComparator> mMaterials;

};

} // namespace Gleam
