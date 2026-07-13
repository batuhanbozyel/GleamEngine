#pragma once
#include "AssetPackage.h"

namespace GEditor {

struct MaterialShaderVariant
{
	Gleam::TStringView name;
	Gleam::TStringView entryPoint;
	Gleam::TArray<Gleam::TStringView> defines;
	Gleam::TArray<Gleam::TStringView> includes;
};

class MaterialSource : public AssetPackage
{
public:
	AssetPackageType(MaterialSource);

	struct ImportSettings
	{
		
	};

	bool Import(const Gleam::Path& path, const ImportSettings& settings);

private:

	bool CompileShaderVariant(const Gleam::Path& path, const Gleam::TString& surfaceShader, const MaterialShaderVariant& variant);
};

} // namespace GEditor
