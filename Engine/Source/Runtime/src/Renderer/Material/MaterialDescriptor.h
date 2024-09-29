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
	TString surfaceShader;
	BlendState blendState{};
	DepthState depthState{};
	StencilState stencilState{};
	CullMode cullingMode = CullMode::Off;
	TArray<MaterialProperty> properties;
};

struct MaterialInstanceDescriptor
{
	TString name;
	AssetReference material;
	TArray<MaterialProperty> properties;

	MaterialPropertyValue& operator[](const TString& name)
	{
		for (auto& property : properties)
		{
			if (property.name == name)
			{
				return property.value;
			}
		}
		GLEAM_ASSERT(false, "Material instance does not have the property {0}", name);
		static MaterialPropertyValue value;
		return value;
	}
};

} // namespace Gleam

template <>
struct std::hash<Gleam::MaterialDescriptor>
{
	size_t operator()(const Gleam::MaterialDescriptor& descriptor) const
	{
		return std::hash<Gleam::TString>()(descriptor.name);
	}
};

GLEAM_TYPE(Gleam::MaterialDescriptor, Guid("37CF7896-D930-435B-A5FF-DF9CEB5C605D"))
	GLEAM_FIELD(name, Serializable())
	GLEAM_FIELD(surfaceShader, Serializable())
	GLEAM_FIELD(blendState, Serializable())
	GLEAM_FIELD(depthState, Serializable())
	GLEAM_FIELD(stencilState, Serializable())
	GLEAM_FIELD(cullingMode, Serializable())
	GLEAM_FIELD(properties, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::MaterialInstanceDescriptor, Guid("910243E7-F9B5-4722-8C77-CB7A81DF4775"))
	GLEAM_FIELD(name, Serializable())
	GLEAM_FIELD(material, Serializable())
	GLEAM_FIELD(properties, Serializable())
GLEAM_END
