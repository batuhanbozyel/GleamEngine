//
//  Material.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "Core/GUID.h"
#include "MaterialProperty.h"
#include "Assets/AssetReference.h"
#include "Renderer/PipelineStateDescriptor.h"

namespace Gleam {

struct MaterialDescriptor
{
	TString name;
	AssetReference surfaceShader;
	AssetReference vertexShader;
	BlendState blendState{};
	DepthState depthState{};
	StencilState stencilState{};
	CullMode cullingMode = CullMode::Off;
	TArray<MaterialProperty> properties;
};

} // namespace Gleam

GLEAM_TYPE(Gleam::MaterialDescriptor, Guid("37CF7896-D930-435B-A5FF-DF9CEB5C605D"))
	GLEAM_FIELD(name, Serializable())
	GLEAM_FIELD(surfaceShader, Serializable())
	GLEAM_FIELD(vertexShader, Serializable())
	GLEAM_FIELD(blendState, Serializable())
	GLEAM_FIELD(depthState, Serializable())
	GLEAM_FIELD(stencilState, Serializable())
	GLEAM_FIELD(cullingMode, Serializable())
	GLEAM_FIELD(properties, Serializable())
GLEAM_END
