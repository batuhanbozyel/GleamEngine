//
//  MaterialInstance.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.04.2023.
//

#include "gpch.h"
#include "MaterialInstance.h"

using namespace Gleam;

MaterialInstance::MaterialInstance(const RefCounted<Material>& parentMaterial)
	: mParentMaterial(parentMaterial)
{

}