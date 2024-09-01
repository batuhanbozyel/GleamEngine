#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

struct RawMaterial
{
    Gleam::TString name;
    Gleam::TString surfaceShader;
    Gleam::TString vertexShader;
    Gleam::BlendState blendState{};
    Gleam::DepthState depthState{};
    Gleam::StencilState stencilState{};
    Gleam::CullMode cullingMode = Gleam::CullMode::Off;
    Gleam::TArray<Gleam::MaterialProperty> properties;
};

struct MaterialSource : AssetPackage
{
	struct ImportSettings
	{
		
	};

	bool Import(const Gleam::Filesystem::Path& path, const ImportSettings& settings);
};

} // namespace GEditor
