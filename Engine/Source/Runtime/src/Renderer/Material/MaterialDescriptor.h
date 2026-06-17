//
//  Material.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "MaterialProperty.h"
#include "Assets/AssetReference.h"
#include "Renderer/PipelineStateDescriptor.h"

namespace Gleam {

GENUM(AlphaMode, "A1F8D3E5-7C2B-4F69-9D08-3E5A8F1B6C72", Serializable)
{
	GITEM(Opaque, "D2A6F3B9-1E58-4C07-B937-8F4D5E2A1C63"),
	GITEM(Mask, "E8F1B4C7-3A92-4D5E-9F02-6B8D7E1A4C29"),
	GITEM(Blend, "F4D9C2A6-7B58-4E13-8F95-2D6A8B9C1E37")
};

GSTRUCT(MaterialDescriptor, "37CF7896-D930-435B-A5FF-DF9CEB5C605D", Serializable)
{
	GFIELD("94B20C24-ECA3-4C3F-B1FA-E5A3A4D2B1D2", Serializable)
	TString name;

	GFIELD("A9F80D30-4A28-4ED2-8B13-C26ECAE4A07E", Serializable)
	TString surfaceShader;

	GFIELD("B5C3E9A1-4F76-48D2-A019-7B6E2D9F1C84", Serializable)
	AlphaMode alphaMode = AlphaMode::Opaque;

	GFIELD("5D83FA61-10A7-463A-B5B0-2BC1B4AB7EF0", Serializable)
	CullMode cullingMode = CullMode::Off;

	GFIELD("F45CEF18-2B8A-403B-87BC-F3E1A1C4CDEE", Serializable)
	TArray<MaterialProperty> properties;
};

GSTRUCT(MaterialInstanceDescriptor, "910243E7-F9B5-4722-8C77-CB7A81DF4775", Serializable)
{
	GFIELD("D5E17A55-48B9-40D3-8C61-1C3F92D6A9D7", Serializable)
	TString name;

	GFIELD("7A9D51B4-E1C0-4CC8-9A33-F3B6EB97A2E3", Serializable)
	AssetReference material;

	GFIELD("EAC82FB3-1DAD-4D87-8E42-13BFBF3D4A2F", Serializable)
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

template <>
struct eastl::hash<Gleam::MaterialDescriptor>
{
	size_t operator()(const Gleam::MaterialDescriptor& descriptor) const
	{
		return std::hash<Gleam::MaterialDescriptor>()(descriptor);
	}
};
