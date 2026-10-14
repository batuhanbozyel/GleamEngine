#include "ShaderCompiler.h"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace GShaderCompiler;

namespace {

struct Options
{
	std::vector<std::filesystem::path> directories;
	std::vector<std::filesystem::path> files;
	std::vector<std::filesystem::path> excludeDirectories;
	std::vector<std::filesystem::path> includeDirectories;
	std::vector<std::string> defines;
	std::filesystem::path outputDirectory;
	bool debug = false;
};

bool ParseOptions(int argc, char** argv, Options& options)
{
	std::string option;
	for (int i = 1; i < argc; ++i)
	{
		const std::string argument = argv[i];
		if (argument == "--debug")
		{
			options.debug = true;
			option.clear();
		}
		else if (argument.starts_with('-'))
		{
			option = argument;
		}
		else if (option == "-d")
		{
			options.directories.emplace_back(argument);
		}
		else if (option == "-f")
		{
			options.files.emplace_back(argument);
		}
		else if (option == "--exclude")
		{
			options.excludeDirectories.emplace_back(argument);
		}
		else if (option == "-I")
		{
			options.includeDirectories.emplace_back(argument);
		}
		else if (option == "-D")
		{
			options.defines.emplace_back(argument);
		}
		else if (option == "-o")
		{
			options.outputDirectory = argument;
		}
		else
		{
			std::cerr << "Error: Unexpected argument '" << argument << "'\n";
			return false;
		}
	}
	return true;
}

bool IsExcluded(const std::filesystem::path& file, const Options& options)
{
	for (const auto& excludeDirectory : options.excludeDirectories)
	{
		const auto relative = file.lexically_normal().lexically_relative(excludeDirectory.lexically_normal());
		if (relative.empty() == false and *relative.begin() != "..")
		{
			return true;
		}
	}
	return false;
}

std::vector<std::filesystem::path> CollectFiles(const Options& options)
{
	std::vector<std::filesystem::path> files = options.files;
	for (const auto& directory : options.directories)
	{
		for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
		{
			if (entry.is_regular_file() and entry.path().extension() == ".hlsl" and IsExcluded(entry.path(), options) == false)
			{
				files.push_back(entry.path());
			}
		}
	}
	return files;
}

std::string ReadFile(const std::filesystem::path& path)
{
	std::ifstream file(path);
	std::stringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

bool CompileFile(const std::filesystem::path& file, const Options& options)
{
	bool succeeded = true;

	const auto source = ReadFile(file);
	for (const auto& entryPoint : FindEntryPoints(source))
	{
		std::cout << "Compiling " << file.filename().string() << " [" << ToString(entryPoint.stage) << "] " << entryPoint.name << "\n";

		CompileDesc desc{
			.source = source,
			.sourcePath = file,
			.entryPoint = entryPoint,
			.exportName = entryPoint.name,
			.defines = options.defines,
			.includeDirectories = { file.parent_path() },
			.debug = options.debug
		};
		desc.includeDirectories.insert(desc.includeDirectories.end(), options.includeDirectories.begin(), options.includeDirectories.end());

		const auto result = Compile(desc);
		if (result.succeeded)
		{
			std::ofstream output(options.outputDirectory / (entryPoint.name + ".dxil"), std::ios::binary);
			output.write(reinterpret_cast<const char*>(result.binary.data()), static_cast<std::streamsize>(result.binary.size()));
		}
		else
		{
			std::cerr << result.errors << "\n";
			succeeded = false;
		}
	}
	return succeeded;
}

} // namespace

int main(int argc, char** argv)
{
	int exitCode = 1;

	Options options;
	if (ParseOptions(argc, argv, options))
	{
		const auto files = CollectFiles(options);
		if (options.outputDirectory.empty())
		{
			std::cerr << "Error: You must specify an output directory with -o.\n";
		}
		else if (files.empty())
		{
			std::cerr << "Error: No HLSL files found.\n";
		}
		else
		{
			std::filesystem::create_directories(options.outputDirectory);

			bool succeeded = true;
			for (const auto& file : files)
			{
				succeeded = CompileFile(file, options) and succeeded;
			}
			exitCode = succeeded ? 0 : 1;
		}
	}
	return exitCode;
}
