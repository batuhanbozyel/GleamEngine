#include "Gleam.h"
#include "MaterialSource.h"
#include "Bakers/MaterialBaker.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

using namespace GEditor;

static Gleam::BlendMode BlendModeFromObject(const rapidjson::Value& object)
{
	Gleam::TString value = object.GetString();
	if (value == "One")
	{
		return Gleam::BlendMode::One;
	}
	else if (value == "Zero")
	{
		return Gleam::BlendMode::Zero;
	}
	else if (value == "DstColor")
	{
		return Gleam::BlendMode::DstColor;
	}
	else if (value == "SrcColor")
	{
		return Gleam::BlendMode::SrcColor;
	}
	else if (value == "OneMinusDstColor")
	{
		return Gleam::BlendMode::OneMinusDstColor;
	}
	else if (value == "OneMinusSrcColor")
	{
		return Gleam::BlendMode::OneMinusSrcColor;
	}
	else if (value == "DstAlpha")
	{
		return Gleam::BlendMode::DstAlpha;
	}
	else if (value == "OneMinusDstAlpha")
	{
		return Gleam::BlendMode::OneMinusDstAlpha;
	}
	else if (value == "SrcAlphaClamp")
	{
		return Gleam::BlendMode::SrcAlphaClamp;
	}
	else if (value == "OneMinusSrcAlpha")
	{
		return Gleam::BlendMode::OneMinusSrcAlpha;
	}
	else
	{
		return Gleam::BlendMode::One;
	}
}

bool MaterialSource::Import(const Gleam::Filesystem::Path& path, const ImportSettings& settings)
{
    auto file = Gleam::Filesystem::Open(path, Gleam::FileType::Text);
    if (file.Empty())
    {
        return false;
    }
    
    rapidjson::IStreamWrapper ss(file.GetStream());
    rapidjson::Document document;
    document.ParseStream(ss);
    
    Gleam::MaterialDescriptor descriptor;
    descriptor.name = path.stem().string();
    
    Gleam::TStringStream generatedShader;
    generatedShader << "struct MaterialProperties\n{\n";
    if (document.HasMember("Properties") && document["Properties"].IsArray())
    {
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
						.value = Gleam::AssetReference() });
                }
            }
        }
        generatedShader << "};\n\n";
        generatedShader << "static MaterialProperties Material = resources.materialBuffer.Load<MaterialProperties>(resources.materialID);\n\n";
    }
    
    if (document.HasMember("SurfaceShader"))
    {
        auto shaderPath = path;
        shaderPath.remove_filename();
        shaderPath /= document["SurfaceShader"].GetString();
        descriptor.surfaceShader = shaderPath.stem().string();
        
        if (shaderPath.has_extension() == false)
        {
            shaderPath += ".shader";
        }
        
        auto generatedPath = shaderPath;
        generatedPath.concat(".gen.hlsl");
        {
            auto shaderFile = Gleam::Filesystem::Open(shaderPath, Gleam::FileType::Text);
            generatedShader << shaderFile.Read();
            
            auto generatedFile = Gleam::Filesystem::Create(generatedPath, Gleam::FileType::Text);
            generatedFile.Write(generatedShader.str());
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
        
        Gleam::TStringStream cmd;
        cmd << PYTHON_INTERPRETER << " ";
        cmd << Gleam::Globals::StartupDirectory/"Tools/CompileShaders.py";
        cmd << " -f " << generatedPath;
        cmd << " -o " << descriptor.surfaceShader;
        cmd << " -i " << "MeshShading.hlsli";
        bool success = ExecuteCommand(cmd.str());
        Gleam::Filesystem::Remove(generatedPath);

		if (success == false)
		{
			return false;
		}
    }
    
    if (document.HasMember("ZWrite"))
    {
        Gleam::TString value = document["ZWrite"].GetString();
        descriptor.depthState.writeEnabled = value == "On";
    }
    
    if (document.HasMember("ZTest"))
    {
        Gleam::TString value = document["ZTest"].GetString();
        if (value == "Never")
        {
            descriptor.depthState.compareFunction = Gleam::CompareFunction::Never;
        }
        else if (value == "Less")
        {
            descriptor.depthState.compareFunction = Gleam::CompareFunction::Less;
        }
        else if (value == "Equal")
        {
            descriptor.depthState.compareFunction = Gleam::CompareFunction::Equal;
        }
        else if (value == "LessEqual")
        {
            descriptor.depthState.compareFunction = Gleam::CompareFunction::LessEqual;
        }
        else if (value == "Greater")
        {
            descriptor.depthState.compareFunction = Gleam::CompareFunction::Greater;
        }
        else if (value == "NotEqual")
        {
            descriptor.depthState.compareFunction = Gleam::CompareFunction::NotEqual;
        }
        else if (value == "GreaterEqual")
        {
            descriptor.depthState.compareFunction = Gleam::CompareFunction::GreaterEqual;
        }
        else if (value == "Always")
        {
            descriptor.depthState.compareFunction = Gleam::CompareFunction::Always;
        }
    }
    
    if (document.HasMember("Cull"))
    {
        Gleam::TString value = document["Cull"].GetString();
        if (value == "Off")
        {
            descriptor.cullingMode = Gleam::CullMode::Off;
        }
        else if (value == "Front")
        {
            descriptor.cullingMode = Gleam::CullMode::Front;
        }
        else if (value == "Back")
        {
            descriptor.cullingMode = Gleam::CullMode::Back;
        }
    }
    
	if (document.HasMember("Blend"))
	{
		auto array = document["Blend"].GetArray();
		if (array.Size() == 2)
		{
			auto srcBlendMode = BlendModeFromObject(array[0]);
			auto dstBlendMode = BlendModeFromObject(array[1]);

			descriptor.blendState.enabled = true;

			descriptor.blendState.sourceColorBlendMode = srcBlendMode;
			descriptor.blendState.sourceAlphaBlendMode = srcBlendMode;

			descriptor.blendState.destinationColorBlendMode = dstBlendMode;
			descriptor.blendState.destinationAlphaBlendMode = dstBlendMode;
		}
		else if (array.Size() == 4)
		{
			descriptor.blendState.enabled = true;

			descriptor.blendState.sourceColorBlendMode = BlendModeFromObject(array[0]);
			descriptor.blendState.sourceAlphaBlendMode = BlendModeFromObject(array[1]);

			descriptor.blendState.destinationColorBlendMode = BlendModeFromObject(array[2]);
			descriptor.blendState.destinationAlphaBlendMode = BlendModeFromObject(array[3]);
		}
	}

	if (document.HasMember("BlendOp"))
	{
		Gleam::TString value = document["BlendOp"].GetString();
		if (value == "Add")
		{
			descriptor.blendState.enabled = true;
			descriptor.blendState.colorBlendOperation = Gleam::BlendOp::Add;
			descriptor.blendState.alphaBlendOperation = Gleam::BlendOp::Add;
		}
		else if (value == "Subtract")
		{
			descriptor.blendState.enabled = true;
			descriptor.blendState.colorBlendOperation = Gleam::BlendOp::Subtract;
			descriptor.blendState.alphaBlendOperation = Gleam::BlendOp::Subtract;
		}
		else if (value == "ReverseSubtract")
		{
			descriptor.blendState.enabled = true;
			descriptor.blendState.colorBlendOperation = Gleam::BlendOp::ReverseSubtract;
			descriptor.blendState.alphaBlendOperation = Gleam::BlendOp::ReverseSubtract;
		}
		else if (value == "Min")
		{
			descriptor.blendState.enabled = true;
			descriptor.blendState.colorBlendOperation = Gleam::BlendOp::Min;
			descriptor.blendState.alphaBlendOperation = Gleam::BlendOp::Min;
		}
		else if (value == "Max")
		{
			descriptor.blendState.enabled = true;
			descriptor.blendState.colorBlendOperation = Gleam::BlendOp::Max;
			descriptor.blendState.alphaBlendOperation = Gleam::BlendOp::Max;
		}
	}
    
    bakers.emplace_back(Gleam::CreateRef<MaterialBaker>(descriptor));
    return true;
}
