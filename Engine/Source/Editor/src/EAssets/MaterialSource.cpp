#include "MaterialSource.h"
#include "Bakers/MaterialBaker.h"

#include "Core/Globals.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

using namespace GEditor;

bool MaterialSource::Import(const Gleam::Path& path, const ImportSettings& settings)
{
    auto file = Gleam::Filesystem::OpenRead(path, Gleam::FileType::Text);
    if (file->Empty())
    {
        return false;
    }
    
    rapidjson::IStreamWrapper ss(file->GetStream());
    rapidjson::Document document;
    document.ParseStream(ss);
    
    Gleam::MaterialDescriptor descriptor;
    descriptor.name = path.Stem();
    
	if (document.HasMember("AlphaMode"))
	{
		Gleam::TString value = document["AlphaMode"].GetString();
		if (value == "Mask")
		{
			descriptor.alphaMode = Gleam::AlphaMode::Mask;
		}
		else if (value == "Blend")
		{
			descriptor.alphaMode = Gleam::AlphaMode::Blend;
		}
		else
		{
			descriptor.alphaMode = Gleam::AlphaMode::Opaque;
		}
	}

	if (document.HasMember("SurfaceShader"))
	{
		Gleam::TStringStream generatedShader;
		if (document.HasMember("Properties") && document["Properties"].IsArray())
		{
			generatedShader << "\n\nstruct MaterialProperties\n{\n";
			for (const auto& property : document["Properties"].GetArray())
			{
				for (auto it = property.MemberBegin(); it != property.MemberEnd(); ++it)
				{
					Gleam::TString propertyName = it->name.GetString();
					Gleam::TString propertyType = it->value.GetString();

					if (propertyType == "Float")
					{
						generatedShader << "\tfloat " << propertyName << ";\n";
						descriptor.properties.emplace_back(Gleam::MaterialProperty{
							.name = propertyName,
							.type = Gleam::MaterialPropertyType::Scalar,
							.value = 0.0f });
					}
					else if (propertyType == "Float2")
					{
						generatedShader << "\tfloat2 " << propertyName << ";\n";
						descriptor.properties.emplace_back(Gleam::MaterialProperty{
							.name = propertyName,
							.type = Gleam::MaterialPropertyType::Float2,
							.value = Gleam::Float2() });
					}
					else if (propertyType == "Float3")
					{
						generatedShader << "\tfloat3 " << propertyName << ";\n";
						descriptor.properties.emplace_back(Gleam::MaterialProperty{
							.name = propertyName,
							.type = Gleam::MaterialPropertyType::Float3,
							.value = Gleam::Float3() });
					}
					else if (propertyType == "Float4")
					{
						generatedShader << "\tfloat4 " << propertyName << ";\n";
						descriptor.properties.emplace_back(Gleam::MaterialProperty{
							.name = propertyName,
							.type = Gleam::MaterialPropertyType::Float4,
							.value = Gleam::Float4() });
					}
					else if (propertyType == "Texture2D")
					{
						generatedShader << "\tGleam::Texture2DResourceView<float4> " << propertyName << ";\n";
						descriptor.properties.emplace_back(Gleam::MaterialProperty{
							.name = propertyName,
							.type = Gleam::MaterialPropertyType::Texture2D,
							.value = Gleam::AssetReference{} });
					}
				}
			}
			generatedShader << "};\n\n";
			generatedShader << "static MaterialProperties Material;\n";
			generatedShader << "void LoadMaterialInstance(ByteAddressBuffer materialBuffer, uint materialID) { Material = materialBuffer.Load<MaterialProperties>(materialID * sizeof(MaterialProperties)); }\n\n";
		}
		else
		{
			generatedShader << "void LoadMaterialInstance(ByteAddressBuffer materialBuffer, uint materialID) {}\n\n";
		}

		auto shaderPath = path;
		shaderPath.RemoveFilename();
		shaderPath /= document["SurfaceShader"].GetString();
		descriptor.surfaceShader = shaderPath.Stem();

		if (shaderPath.HasExtension() == false)
		{
			shaderPath.Concat(".shader");
		}

		{
			auto shaderFile = Gleam::Filesystem::OpenRead(shaderPath, Gleam::FileType::Text);
			auto shaderContents = shaderFile->Read();
			generatedShader << shaderContents << "\0";
		}

		// Compile material shader
		{
			auto generatedPath = shaderPath;
			generatedPath.Concat(".gen.hlsl");
			{
				auto generatedFile = Gleam::Filesystem::Create(generatedPath, Gleam::FileType::Text);
				generatedFile->Write(generatedShader.str());
			}

			if (document.HasMember("Lighting"))
			{
				Gleam::TString value = document["Lighting"].GetString();
				if (value == "On")
				{
					// TODO: generate lit shader
				}
				else
				{
					// TODO: generate unlit shader
				}
			}

			// Depth prepass
			if (CompileShaderVariant(generatedPath, descriptor.surfaceShader, {
					.name = "DepthPrepass",
					.entryPoint = "main",
					.defines = {},
					.includes = { "DepthPrepass.hlsli" }
				}) == false)
			{
				return false;
			}

			// Raster shading
			if (CompileShaderVariant(generatedPath, descriptor.surfaceShader, {
					.name = "Shading",
					.entryPoint = "main",
					.defines = {},
					.includes = { "MeshShading.hlsli" }
				}) == false)
			{
				return false;
			}

			// Closest hit
			if (CompileShaderVariant(generatedPath, descriptor.surfaceShader, {
					.name = "ClosestHit",
					.entryPoint = "ClosestHit",
					.defines = {},
					.includes = { "PathTraceShading.hlsli" }
				}) == false)
			{
				return false;
			}

			// Any hit
			if (CompileShaderVariant(generatedPath, descriptor.surfaceShader, {
					.name = "AnyHit",
					.entryPoint = "AnyHit",
					.defines = {},
					.includes = { "PathTraceShading.hlsli" }
				}) == false)
			{
				return false;
			}

			// Shadow any hit
			if (CompileShaderVariant(generatedPath, descriptor.surfaceShader, {
					.name = "ShadowAnyHit",
					.entryPoint = "ShadowAnyHit",
					.defines = {},
					.includes = { "PathTraceShading.hlsli" }
				}) == false)
			{
				return false;
			}

			// Reflection closest hit
			if (CompileShaderVariant(generatedPath, descriptor.surfaceShader, {
					.name = "ReflectionClosestHit",
					.entryPoint = "ClosestHit",
					.defines = {},
					.includes = { "Reflection/RayTracedReflectionShading.hlsli" }
				}) == false)
			{
				return false;
			}

			// Reflection any hit
			if (CompileShaderVariant(generatedPath, descriptor.surfaceShader, {
					.name = "ReflectionAnyHit",
					.entryPoint = "AnyHit",
					.defines = {},
					.includes = { "Reflection/RayTracedReflectionShading.hlsli" }
				}) == false)
			{
				return false;
			}

			// Visibility shading
			if (CompileShaderVariant(generatedPath, descriptor.surfaceShader, {
					.name = "VisibilityShading",
					.entryPoint = "main",
					.defines = {},
					.includes = { "VisibilityShading.hlsli" }
				}) == false)
			{
				return false;
			}

			// GBuffer resolve
			if (CompileShaderVariant(generatedPath, descriptor.surfaceShader, {
					.name = "GBufferResolve",
					.entryPoint = "main",
					.defines = {},
					.includes = { "GBufferResolve.hlsli" }
				}) == false)
			{
				return false;
			}

			Gleam::Filesystem::Remove(generatedPath);
		}
	}
    
    if (document.HasMember("Cull"))
    {
        Gleam::TString value = document["Cull"].GetString();
		if (value == "Back")
		{
			descriptor.cullingMode = Gleam::CullMode::Back;
		}
		else if (value == "Front")
		{
			descriptor.cullingMode = Gleam::CullMode::Front;
		}
		else
		{
			descriptor.cullingMode = Gleam::CullMode::Off;
		}
    }

    EmplaceBaker<MaterialBaker>(descriptor);
    return true;
}

bool MaterialSource::CompileShaderVariant(const Gleam::Path& path, const Gleam::TString& surfaceShader, const MaterialShaderVariant& variant)
{
	Gleam::Path dxilShader = Gleam::Globals::BuiltinAssetsDirectory / "Shaders" / (surfaceShader + variant.name);
	dxilShader.Concat(".dxil");
	if (Gleam::Filesystem::Exists(dxilShader))
	{
		Gleam::Filesystem::Remove(dxilShader);
	}

	Gleam::TStringStream cmd;
	cmd << PYTHON_INTERPRETER << " ";
	cmd << Gleam::Globals::StartupDirectory / "Tools/CompileShaders.py";
	cmd << " -f " << path;

	for (auto define : variant.defines)
	{
		cmd << " -D " << define;
	}

	for (auto include : variant.includes)
	{
		cmd << " -i " << include;
	}

	cmd << " --entry " << variant.entryPoint << "=" << surfaceShader << variant.name;
#ifdef GDEBUG
	cmd << " --debug";
#endif

	if (ExecuteCommand(cmd.str()) != 0)
	{
		Gleam::Filesystem::Remove(path);
		return false;
	}
	return true;
}
