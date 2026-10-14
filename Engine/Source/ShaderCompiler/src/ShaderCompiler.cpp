#include "ShaderCompiler.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <dxcapi.h>

#include <regex>

using namespace GShaderCompiler;

namespace {

struct StageInfo
{
	ShaderStage stage;
	const char* name;
	const wchar_t* profile;
	const wchar_t* targetDefine;
	bool library;
};

constexpr StageInfo StageInfos[] = {
	{ ShaderStage::Vertex, "vertex", L"vs_6_6", L"SHADER_TARGET_VERTEX", false },
	{ ShaderStage::Pixel, "pixel", L"ps_6_6", L"SHADER_TARGET_PIXEL", false },
	{ ShaderStage::Compute, "compute", L"cs_6_6", L"SHADER_TARGET_COMPUTE", false },
	{ ShaderStage::RayGeneration, "raygeneration", L"lib_6_6", L"SHADER_TARGET_RAYGENERATION", true },
	{ ShaderStage::Miss, "miss", L"lib_6_6", L"SHADER_TARGET_MISS", true },
	{ ShaderStage::ClosestHit, "closesthit", L"lib_6_6", L"SHADER_TARGET_CLOSESTHIT", true },
	{ ShaderStage::AnyHit, "anyhit", L"lib_6_6", L"SHADER_TARGET_ANYHIT", true },
	{ ShaderStage::Intersection, "intersection", L"lib_6_6", L"SHADER_TARGET_INTERSECTION", true },
	{ ShaderStage::Callable, "callable", L"lib_6_6", L"SHADER_TARGET_CALLABLE", true },
	{ ShaderStage::Mesh, "mesh", L"ms_6_6", L"SHADER_TARGET_MESH", false },
	{ ShaderStage::Amplification, "amplification", L"as_6_6", L"SHADER_TARGET_AMPLIFICATION", false }
};

#ifdef __APPLE__
constexpr const wchar_t* RendererApiDefine = L"USE_METAL_RENDERER";
#else
constexpr const wchar_t* RendererApiDefine = L"USE_DIRECTX_RENDERER";
#endif

const StageInfo& GetStageInfo(ShaderStage stage)
{
	return StageInfos[static_cast<size_t>(stage)];
}

const StageInfo* FindStageInfo(const std::string& name)
{
	for (const auto& stageInfo : StageInfos)
	{
		if (name == stageInfo.name)
		{
			return &stageInfo;
		}
	}
	return nullptr;
}

std::wstring Widen(const std::string& str)
{
	return std::wstring(str.begin(), str.end());
}

std::regex BuildShaderAttributeRegex()
{
	std::string stages;
	for (const auto& stageInfo : StageInfos)
	{
		if (stages.empty() == false)
		{
			stages += '|';
		}
		stages += stageInfo.name;
	}

	return std::regex(
		R"re(\[shader\("()re" + stages + R"re()"\)\])re"
		R"((?:\s*\[[^\]]*\])*)"
		R"(\s+)"
		R"((?:[\w:]+\s+)*?)"
		R"((\w+)\s*\()");
}

DxcCreateInstanceProc GetCreateInstance()
{
#ifdef _WIN32
	static HMODULE module = LoadLibraryW(L"dxcompiler.dll");
	static DxcCreateInstanceProc createInstance = module != nullptr ? reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(module, "DxcCreateInstance")) : nullptr;
	return createInstance;
#else
	return &DxcCreateInstance;
#endif
}

std::vector<std::wstring> BuildArguments(const CompileDesc& desc)
{
	const auto& stageInfo = GetStageInfo(desc.entryPoint.stage);

	std::vector<std::wstring> arguments = {
		desc.sourcePath.wstring(),
		L"-HV", L"2021",
		L"-enable-16bit-types",
		L"-D", RendererApiDefine,
		L"-D", stageInfo.targetDefine,
		L"-T", stageInfo.profile
	};

	for (const auto& define : desc.defines)
	{
		arguments.insert(arguments.end(), { L"-D", Widen(define) });
	}

	if (stageInfo.library)
	{
		arguments.insert(arguments.end(), { L"-exports", Widen(desc.exportName + "=" + desc.entryPoint.name) });
	}
	else
	{
		if (desc.exportName != desc.entryPoint.name)
		{
			arguments.insert(arguments.end(), { L"-D", Widen(desc.entryPoint.name + "=" + desc.exportName) });
		}
		arguments.insert(arguments.end(), { L"-E", Widen(desc.exportName) });
	}

	if (desc.debug)
	{
		arguments.insert(arguments.end(), { L"-Zi", L"-Qembed_debug" });
	}

	for (const auto& includeDirectory : desc.includeDirectories)
	{
		arguments.insert(arguments.end(), { L"-I", includeDirectory.wstring() });
	}
	return arguments;
}

void CompileWithDxc(IDxcCompiler3* compiler, IDxcIncludeHandler* includeHandler, const CompileDesc& desc, CompileResult& result)
{
	const auto arguments = BuildArguments(desc);
	std::vector<LPCWSTR> argumentPointers;
	for (const auto& argument : arguments)
	{
		argumentPointers.push_back(argument.c_str());
	}

	DxcBuffer sourceBuffer{
		.Ptr = desc.source.data(),
		.Size = desc.source.size(),
		.Encoding = DXC_CP_UTF8
	};

	IDxcResult* compileResult = nullptr;
	if (SUCCEEDED(compiler->Compile(&sourceBuffer, argumentPointers.data(), static_cast<UINT32>(argumentPointers.size()), includeHandler, IID_PPV_ARGS(&compileResult))))
	{
		IDxcBlobUtf8* errors = nullptr;
		if (SUCCEEDED(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr)) and errors != nullptr)
		{
			result.errors.assign(errors->GetStringPointer(), errors->GetStringLength());
			errors->Release();
		}

		HRESULT status = E_FAIL;
		compileResult->GetStatus(&status);

		IDxcBlob* object = nullptr;
		if (SUCCEEDED(status) and SUCCEEDED(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&object), nullptr)) and object != nullptr)
		{
			const auto* bytes = static_cast<const uint8_t*>(object->GetBufferPointer());
			result.binary.assign(bytes, bytes + object->GetBufferSize());
			result.succeeded = true;
			object->Release();
		}
		compileResult->Release();
	}
	else
	{
		result.errors = "DirectX Shader Compiler failed to run";
	}
}

} // namespace

const char* GShaderCompiler::ToString(ShaderStage stage)
{
	return GetStageInfo(stage).name;
}

std::vector<EntryPoint> GShaderCompiler::FindEntryPoints(const std::string& source)
{
	static const std::regex shaderAttribute = BuildShaderAttributeRegex();

	std::vector<EntryPoint> entryPoints;
	for (auto it = std::sregex_iterator(source.begin(), source.end(), shaderAttribute); it != std::sregex_iterator(); ++it)
	{
		const auto& match = *it;
		entryPoints.push_back(EntryPoint{ .stage = FindStageInfo(match[1].str())->stage, .name = match[2].str() });
	}
	return entryPoints;
}

CompileResult GShaderCompiler::Compile(const CompileDesc& desc)
{
	CompileResult result;

	auto createInstance = GetCreateInstance();
	if (createInstance == nullptr)
	{
		result.errors = "DirectX Shader Compiler library could not be loaded";
	}
	else
	{
		IDxcUtils* utils = nullptr;
		IDxcCompiler3* compiler = nullptr;
		IDxcIncludeHandler* includeHandler = nullptr;
		const bool created = SUCCEEDED(createInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)))
			and SUCCEEDED(createInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)))
			and SUCCEEDED(utils->CreateDefaultIncludeHandler(&includeHandler));

		if (created)
		{
			CompileWithDxc(compiler, includeHandler, desc, result);
		}
		else
		{
			result.errors = "DirectX Shader Compiler could not be initialized";
		}

		if (includeHandler != nullptr)
		{
			includeHandler->Release();
		}
		if (compiler != nullptr)
		{
			compiler->Release();
		}
		if (utils != nullptr)
		{
			utils->Release();
		}
	}
	return result;
}
