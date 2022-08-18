#pragma once
#include <fstream>

namespace Gleam {

namespace IOUtils {

static TArray<uint8_t> ReadBinaryFile(const Filesystem::path& filepath)
{
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	GLEAM_ASSERT(file.is_open());

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	TArray<uint8_t> buffer(size);

	file.seekg(0);
	file.read(As<char*>(buffer.data()), size);

	return buffer;
}

static TString ReadFile(const Filesystem::path& filepath)
{
	std::ifstream file(filepath);
	GLEAM_ASSERT(file.is_open());

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
    TString buffer(size, 0);

	file.seekg(0);
	file.read(buffer.data(), size);

	return buffer;
}

static void ExecuteCommand(const TString& cmd)
{
	int success = system((cmd + " > command.err 2>&1").c_str());
	GLEAM_ASSERT(success == 0);
}

TArray<Filesystem::path> OpenFileDialog();

} // namespace FileUtils

} // namespace Gleam
