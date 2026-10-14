#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace GShaderCompiler {

enum class ShaderStage
{
	Vertex,
	Pixel,
	Compute,
	RayGeneration,
	Miss,
	ClosestHit,
	AnyHit,
	Intersection,
	Callable,
	Mesh,
	Amplification
};

struct EntryPoint
{
	ShaderStage stage = ShaderStage::Pixel;
	std::string name;
};

struct CompileDesc
{
	std::string source;
	std::filesystem::path sourcePath;
	EntryPoint entryPoint;
	std::string exportName;
	std::vector<std::string> defines;
	std::vector<std::filesystem::path> includeDirectories;
	bool debug = false;
};

struct CompileResult
{
	bool succeeded = false;
	std::vector<uint8_t> binary;
	std::string errors;
};

const char* ToString(ShaderStage stage);

std::vector<EntryPoint> FindEntryPoints(const std::string& source);

CompileResult Compile(const CompileDesc& desc);

} // namespace GShaderCompiler
