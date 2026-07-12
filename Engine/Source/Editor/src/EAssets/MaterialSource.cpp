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
			{
				Gleam::Path dxilShader = Gleam::Globals::BuiltinAssetsDirectory / "Shaders" / (descriptor.surfaceShader + "DepthPrepass");
				dxilShader.Concat(".dxil");
				if (Gleam::Filesystem::Exists(dxilShader))
				{
					Gleam::Filesystem::Remove(dxilShader);
				}

				Gleam::TStringStream cmd;
				cmd << PYTHON_INTERPRETER << " ";
				cmd << Gleam::Globals::StartupDirectory / "Tools/CompileShaders.py";
				cmd << " -f " << generatedPath;
				cmd << " -i " << "DepthPrepass.hlsli";
				cmd << " --entry main=" << descriptor.surfaceShader << "DepthPrepass";
			#ifdef GDEBUG
				cmd << " --debug";
			#endif

				if (ExecuteCommand(cmd.str()) != 0)
				{
					Gleam::Filesystem::Remove(generatedPath);
					return false;
				}
			}

			// Raster shading
			{
				Gleam::Path dxilShader = Gleam::Globals::BuiltinAssetsDirectory / "Shaders" / (descriptor.surfaceShader + "Shading");
				dxilShader.Concat(".dxil");
				if (Gleam::Filesystem::Exists(dxilShader))
				{
					Gleam::Filesystem::Remove(dxilShader);
				}

				Gleam::TStringStream cmd;
				cmd << PYTHON_INTERPRETER << " ";
				cmd << Gleam::Globals::StartupDirectory / "Tools/CompileShaders.py";
				cmd << " -f " << generatedPath;
				cmd << " -i " << "MeshShading.hlsli";
				cmd << " --entry main=" << descriptor.surfaceShader << "Shading";
			#ifdef GDEBUG
				cmd << " --debug";
			#endif

				if (ExecuteCommand(cmd.str()) != 0)
				{
					Gleam::Filesystem::Remove(generatedPath);
					return false;
				}
			}

			// Closest hit
			{
				Gleam::Path dxilShader = Gleam::Globals::BuiltinAssetsDirectory / "Shaders" / (descriptor.surfaceShader + "ClosestHit");
				dxilShader.Concat(".dxil");
				if (Gleam::Filesystem::Exists(dxilShader))
				{
					Gleam::Filesystem::Remove(dxilShader);
				}

				Gleam::TStringStream cmd;
				cmd << PYTHON_INTERPRETER << " ";
				cmd << Gleam::Globals::StartupDirectory / "Tools/CompileShaders.py";
				cmd << " -f " << generatedPath;
				cmd << " -i " << "PathTraceShading.hlsli";
				cmd << " --entry ClosestHit=" << descriptor.surfaceShader << "ClosestHit";
			#ifdef GDEBUG
				cmd << " --debug";
			#endif

				if (ExecuteCommand(cmd.str()) != 0)
				{
					Gleam::Filesystem::Remove(generatedPath);
					return false;
				}
			}

			// Any hit
			{
				Gleam::Path dxilShader = Gleam::Globals::BuiltinAssetsDirectory / "Shaders" / (descriptor.surfaceShader + "AnyHit");
				dxilShader.Concat(".dxil");
				if (Gleam::Filesystem::Exists(dxilShader))
				{
					Gleam::Filesystem::Remove(dxilShader);
				}

				Gleam::TStringStream cmd;
				cmd << PYTHON_INTERPRETER << " ";
				cmd << Gleam::Globals::StartupDirectory / "Tools/CompileShaders.py";
				cmd << " -f " << generatedPath;
				cmd << " -i " << "PathTraceShading.hlsli";
				cmd << " --entry AnyHit=" << descriptor.surfaceShader << "AnyHit";
			#ifdef GDEBUG
				cmd << " --debug";
			#endif

				if (ExecuteCommand(cmd.str()) != 0)
				{
					Gleam::Filesystem::Remove(generatedPath);
					return false;
				}
			}

			// Shadow any hit
			{
				Gleam::Path dxilShader = Gleam::Globals::BuiltinAssetsDirectory / "Shaders" / (descriptor.surfaceShader + "ShadowAnyHit");
				dxilShader.Concat(".dxil");
				if (Gleam::Filesystem::Exists(dxilShader))
				{
					Gleam::Filesystem::Remove(dxilShader);
				}

				Gleam::TStringStream cmd;
				cmd << PYTHON_INTERPRETER << " ";
				cmd << Gleam::Globals::StartupDirectory / "Tools/CompileShaders.py";
				cmd << " -f " << generatedPath;
				cmd << " -i " << "PathTraceShading.hlsli";
				cmd << " --entry ShadowAnyHit=" << descriptor.surfaceShader << "ShadowAnyHit";
			#ifdef GDEBUG
				cmd << " --debug";
			#endif

				if (ExecuteCommand(cmd.str()) != 0)
				{
					Gleam::Filesystem::Remove(generatedPath);
					return false;
				}
			}

			// Visibility shading
			{
				Gleam::Path dxilShader = Gleam::Globals::BuiltinAssetsDirectory / "Shaders" / (descriptor.surfaceShader + "VisibilityShading");
				dxilShader.Concat(".dxil");
				if (Gleam::Filesystem::Exists(dxilShader))
				{
					Gleam::Filesystem::Remove(dxilShader);
				}

				Gleam::TStringStream cmd;
				cmd << PYTHON_INTERPRETER << " ";
				cmd << Gleam::Globals::StartupDirectory / "Tools/CompileShaders.py";
				cmd << " -f " << generatedPath;
				cmd << " -i " << "VisibilityShading.hlsli";
				cmd << " --entry main=" << descriptor.surfaceShader << "VisibilityShading";
			#ifdef GDEBUG
				cmd << " --debug";
			#endif

				if (ExecuteCommand(cmd.str()) != 0)
				{
					Gleam::Filesystem::Remove(generatedPath);
					return false;
				}
			}

			// GBuffer resolve
			{
				Gleam::Path dxilShader = Gleam::Globals::BuiltinAssetsDirectory / "Shaders" / (descriptor.surfaceShader + "GBufferResolve");
				dxilShader.Concat(".dxil");
				if (Gleam::Filesystem::Exists(dxilShader))
				{
					Gleam::Filesystem::Remove(dxilShader);
				}

				Gleam::TStringStream cmd;
				cmd << PYTHON_INTERPRETER << " ";
				cmd << Gleam::Globals::StartupDirectory / "Tools/CompileShaders.py";
				cmd << " -f " << generatedPath;
				cmd << " -i " << "GBufferResolve.hlsli";
				cmd << " --entry main=" << descriptor.surfaceShader << "GBufferResolve";
			#ifdef GDEBUG
				cmd << " --debug";
			#endif

				if (ExecuteCommand(cmd.str()) != 0)
				{
					Gleam::Filesystem::Remove(generatedPath);
					return false;
				}
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
